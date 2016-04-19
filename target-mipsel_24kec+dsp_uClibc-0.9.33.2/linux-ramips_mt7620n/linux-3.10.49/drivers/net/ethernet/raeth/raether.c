#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/if_ether.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include <asm/rt2880/rt_mmap.h>
#include "ra2882ethreg.h"
#include "raether.h"
#include "ra_mac.h"
#include "ra_ioctl.h"

static int rt2880_eth_recv(struct net_device* dev);
int reg_dbg = 0;

void setup_internal_gsw(void);

#define	MAX_RX_LENGTH	1536

struct net_device		*dev_raether;

static int rx_dma_owner_idx; 
static int rx_dma_owner_idx0;
static int pending_recv;
static struct PDMA_rxdesc	*rx_ring;
static unsigned long tx_ring_full=0;

#define KSEG1                   0xa0000000
#define PHYS_TO_VIRT(x)         ((void *)((x) | KSEG1))
#define VIRT_TO_PHYS(x)         ((unsigned long)(x) & ~KSEG1)

extern int fe_dma_init(struct net_device *dev);
extern int ei_start_xmit(struct sk_buff* skb, struct net_device *dev, int gmac_no);
extern void ei_xmit_housekeeping(unsigned long unused);
extern inline int rt2880_eth_send(struct net_device* dev, struct sk_buff *skb, int gmac_no);

static int ei_set_mac_addr(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	if(netif_running(dev))
		return -EBUSY;

        ra2880MacAddressSet(addr->sa_data);
	return 0;
}


void set_fe_dma_glo_cfg(void)
{
        int dma_glo_cfg=0;

	dma_glo_cfg = (TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN | PDMA_BT_SIZE_16DWORDS);

	dma_glo_cfg |= (RX_2B_OFFSET);

	sysRegWrite(DMA_GLO_CFG, dma_glo_cfg);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "DMA_GLO_CFG", DMA_GLO_CFG, dma_glo_cfg);
}

int forward_config(struct net_device *dev)
{
	unsigned int	regVal, regCsg;

	regVal = sysRegRead(GDMA1_FWD_CFG);
	regCsg = sysRegRead(CDMA_CSG_CFG);

	//set unicast/multicast/broadcast frame to cpu
	regVal &= ~0xFFFF;
	regVal |= GDMA1_FWD_PORT;
	regCsg &= ~0x7;

	//disable ipv4 header checksum check
	regVal &= ~GDM1_ICS_EN;
	regCsg &= ~ICS_GEN_EN;

	//disable tcp checksum check
	regVal &= ~GDM1_TCS_EN;
	regCsg &= ~TCS_GEN_EN;

	//disable udp checksum check
	regVal &= ~GDM1_UCS_EN;
	regCsg &= ~UCS_GEN_EN;


	dev->features &= ~NETIF_F_IP_CSUM; /* disable checksum TCP/UDP over IPv4 */


	sysRegWrite(GDMA1_FWD_CFG, regVal);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "GDMA1_FWD_CFG", GDMA1_FWD_CFG, regVal);
	sysRegWrite(CDMA_CSG_CFG, regCsg);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "CDMA_CSG_CFG", CDMA_CSG_CFG, regCsg);

	regVal = 0x1;
	sysRegWrite(FE_RST_GL, regVal);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "FE_RST_GL", FE_RST_GL, regVal);
	sysRegWrite(FE_RST_GL, 0);	// update for RSTCTL issue
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "FE_RST_GL", FE_RST_GL, 1);

	regCsg = sysRegRead(CDMA_CSG_CFG);
	printk("CDMA_CSG_CFG = %0X\n",regCsg);
	regVal = sysRegRead(GDMA1_FWD_CFG);
	printk("GDMA1_FWD_CFG = %0X\n",regVal);

	return 1;
}


static int rt2880_eth_recv(struct net_device* dev)
{
	struct sk_buff	*skb, *rx_skb;
	unsigned int	length = 0;
	unsigned long	RxProcessed;


	int bReschedule = 0;
	END_DEVICE* 	ei_local = netdev_priv(dev);



	RxProcessed = 0;

	rx_dma_owner_idx0 = (sysRegRead(RAETH_RX_CALC_IDX0) + 1) % NUM_RX_DESC;

	for ( ; ; ) {

		if (RxProcessed++ > NUM_RX_MAX_PROCESS)
                {
                        // need to reschedule rx handle
                        bReschedule = 1;
                        break;
                }



		if (ei_local->rx_ring0[rx_dma_owner_idx0].rxd_info2.DDONE_bit == 1)  {
		    rx_ring = ei_local->rx_ring0;
		    rx_dma_owner_idx = rx_dma_owner_idx0;
		} else {
		    break;
		}

		/* skb processing */
		length = rx_ring[rx_dma_owner_idx].rxd_info2.PLEN0;
		rx_skb = ei_local->netrx0_skbuf[rx_dma_owner_idx];
		rx_skb->data = ei_local->netrx0_skbuf[rx_dma_owner_idx]->data;
		rx_skb->len 	= length;

		rx_skb->data += NET_IP_ALIGN;

		rx_skb->tail 	= rx_skb->data + length;

		rx_skb->dev 	  = dev;
		rx_skb->protocol  = eth_type_trans(rx_skb,dev);

		    rx_skb->ip_summed = CHECKSUM_NONE;


		/* We have to check the free memory size is big enough
		 * before pass the packet to cpu*/
		skb = __dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN, GFP_ATOMIC);

		if (unlikely(skb == NULL))
		{
			printk(KERN_ERR "skb not available...\n");
				ei_local->stat.rx_dropped++;
                        bReschedule = 1;
			break;
		}

         {
                netif_rx(rx_skb);
         }

		{
			ei_local->stat.rx_packets++;
			ei_local->stat.rx_bytes += length;
		}


		rx_ring[rx_dma_owner_idx].rxd_info2.PLEN0 = MAX_RX_LENGTH;
		rx_ring[rx_dma_owner_idx].rxd_info2.LS0 = 0;
		rx_ring[rx_dma_owner_idx].rxd_info2.DDONE_bit = 0;
		rx_ring[rx_dma_owner_idx].rxd_info1.PDP0 = dma_map_single(NULL, skb->data, MAX_RX_LENGTH, PCI_DMA_FROMDEVICE);

		/*  Move point to next RXD which wants to alloc*/
		sysRegWrite(RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
		if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RAETH_RX_CALC_IDX0", RAETH_RX_CALC_IDX0, rx_dma_owner_idx);
		ei_local->netrx0_skbuf[rx_dma_owner_idx] = skb;

		/* Update to Next packet point that was received.
		 */
		rx_dma_owner_idx0 = (sysRegRead(RAETH_RX_CALC_IDX0) + 1) % NUM_RX_DESC;
	}	/* for */

	return bReschedule;
}

void ei_receive_workq(struct work_struct *work)
{
	struct net_device *dev = dev_raether;
	END_DEVICE *ei_local = netdev_priv(dev);
	unsigned long reg_int_mask=0;
	int bReschedule=0;


	if(tx_ring_full==0){
		bReschedule = rt2880_eth_recv(dev);
		if(bReschedule)
		{
			schedule_work(&ei_local->rx_wq);
		}else{
			reg_int_mask=sysRegRead(RAETH_FE_INT_ENABLE);
			sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask| RX_DLY_INT);
			if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "RAETH_FE_INT_ENABLE", RAETH_FE_INT_ENABLE, reg_int_mask| RX_DLY_INT);
		}
	}else{
                schedule_work(&ei_local->rx_wq);
	}
}


static irqreturn_t ei_interrupt(int irq, void *dev_id)
{
	unsigned long reg_int_val;
	unsigned long reg_int_mask=0;
	unsigned int recv = 0;
	unsigned int transmit __maybe_unused = 0;
	unsigned long flags;

	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local = netdev_priv(dev);

	if (dev == NULL)
	{
		printk (KERN_ERR "net_interrupt(): irq %x for unknown device.\n", IRQ_ENET0);
		return IRQ_NONE;
	}


	spin_lock_irqsave(&(ei_local->page_lock), flags);
	reg_int_val = sysRegRead(RAETH_FE_INT_STATUS);

	if((reg_int_val & RX_DLY_INT))
		recv = 1;
	
	if (reg_int_val & RAETH_TX_DLY_INT)
		transmit = 1;

	sysRegWrite(RAETH_FE_INT_STATUS, RAETH_FE_INT_DLY_INIT);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "RAETH_FE_INT_STATUS", RAETH_FE_INT_STATUS, RAETH_FE_INT_DLY_INIT);

	ei_xmit_housekeeping(0);

	if (((recv == 1) || (pending_recv ==1)) && (tx_ring_full==0))
	{
		reg_int_mask = sysRegRead(RAETH_FE_INT_ENABLE);
		sysRegWrite(RAETH_FE_INT_ENABLE, reg_int_mask & ~(RX_DLY_INT));
		if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "RAETH_FE_INT_ENABLE", RAETH_FE_INT_ENABLE, reg_int_mask & ~(RX_DLY_INT));
		pending_recv=0;
		schedule_work(&ei_local->rx_wq);
	} 
	else if (recv == 1 && tx_ring_full==1) 
	{
		pending_recv=1;
	}
	spin_unlock_irqrestore(&(ei_local->page_lock), flags);

	return IRQ_HANDLED;
}

static void esw_link_status_changed(int port_no, void *dev_id)
{
    unsigned int reg_val;
    mii_mgr_read(31, (0x3008 + (port_no*0x100)), &reg_val);
    if(reg_val & 0x1) {
	printk("ESW: Link Status Changed - Port%d Link UP\n", port_no);
    } else {	    
	printk("ESW: Link Status Changed - Port%d Link Down\n", port_no);
    }
}


static irqreturn_t esw_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned int reg_int_val;
	struct net_device *dev = (struct net_device *) dev_id;
	END_DEVICE *ei_local = netdev_priv(dev);

	spin_lock_irqsave(&(ei_local->page_lock), flags);
        mii_mgr_read(31, 0x700c, &reg_int_val);

	if (reg_int_val & P4_LINK_CH) {
	    esw_link_status_changed(4, dev_id);
	}

	if (reg_int_val & P3_LINK_CH) {
	    esw_link_status_changed(3, dev_id);
	}
	if (reg_int_val & P2_LINK_CH) {
	    esw_link_status_changed(2, dev_id);
	}
	if (reg_int_val & P1_LINK_CH) {
	    esw_link_status_changed(1, dev_id);
	}
	if (reg_int_val & P0_LINK_CH) {
	    esw_link_status_changed(0, dev_id);
	}

        mii_mgr_write(31, 0x700c, 0x1f); //ack switch link change
	spin_unlock_irqrestore(&(ei_local->page_lock), flags);
	return IRQ_HANDLED;
}



static int ei_start_xmit_fake(struct sk_buff* skb, struct net_device *dev)
{
	return ei_start_xmit(skb, dev, 1);
}

static int ei_change_mtu(struct net_device *dev, int new_mtu)
{
	unsigned long flags;
	END_DEVICE *ei_local = netdev_priv(dev);  // get priv ei_local pointer from net_dev structure

	if ( ei_local == NULL ) {
		printk(KERN_EMERG "%s: ei_change_mtu passed a non-existent private pointer from net_dev!\n", dev->name);
		return -ENXIO;
	}

	spin_lock_irqsave(&ei_local->page_lock, flags);

	if ( (new_mtu > 4096) || (new_mtu < 64)) {
		spin_unlock_irqrestore(&ei_local->page_lock, flags);
		return -EINVAL;
	}

	if ( new_mtu > 1500 ) {
		spin_unlock_irqrestore(&ei_local->page_lock, flags);
		return -EINVAL;
	}

	dev->mtu = new_mtu;

	spin_unlock_irqrestore(&ei_local->page_lock, flags);
	return 0;
}


static const struct net_device_ops ei_netdev_ops = {
        .ndo_init               = rather_probe,
        .ndo_open               = ei_open,
        .ndo_stop               = ei_close,
        .ndo_start_xmit         = ei_start_xmit_fake,
        .ndo_set_mac_address    = eth_mac_addr,
        .ndo_change_mtu         = ei_change_mtu,
        .ndo_validate_addr      = eth_validate_addr,
};

void ra2880_setup_dev_fptable(struct net_device *dev)
{
	RAETH_PRINT(__FUNCTION__ "is called!\n");

	dev->netdev_ops		= &ei_netdev_ops;
#define TX_TIMEOUT (5*HZ)
	dev->watchdog_timeo = TX_TIMEOUT;

}

void fe_reset(void)
{
	u32 val;
	val = sysRegRead(RSTCTRL);

	val = val | RALINK_FE_RST;
	sysRegWrite(RSTCTRL, val);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RSTCTRL", RSTCTRL, val);
	val = val & ~(RALINK_FE_RST);
	sysRegWrite(RSTCTRL, val);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RSTCTRL", RSTCTRL, val);
}

void ei_reset_task(struct work_struct *work)
{
	struct net_device *dev = dev_raether;

	ei_close(dev);
	ei_open(dev);

	return;
}

void ei_tx_timeout(struct net_device *dev)
{
        END_DEVICE *ei_local = netdev_priv(dev);

        schedule_work(&ei_local->reset_task);
}

int __init rather_probe(struct net_device *dev)
{
	END_DEVICE *ei_local = netdev_priv(dev);
	struct sockaddr addr;
	unsigned char mac_addr01234[5] = {0x00, 0x0C, 0x43, 0x28, 0x80};

	fe_reset();
	net_srandom(jiffies);
	memcpy(addr.sa_data, mac_addr01234, 5);
	addr.sa_data[5] = net_random()&0xFF;
	ei_set_mac_addr(dev, &addr);
	spin_lock_init(&ei_local->page_lock);
	ether_setup(dev);

	return 0;
}


int ei_open(struct net_device *dev)
{
	int i, err;
	unsigned long flags;
	END_DEVICE *ei_local;


	if (!try_module_get(THIS_MODULE))
	{
		printk("%s: Cannot reserve module\n", __FUNCTION__);
		return -1;
	}
	printk("Raeth %s (",RAETH_VERSION);
	printk("Workqueue");

	printk(")\n");
  	ei_local = netdev_priv(dev); // get device pointer from System
	// unsigned int flags;

	if (ei_local == NULL)
	{
		printk(KERN_EMERG "%s: ei_open passed a non-existent device!\n", dev->name);
		return -ENXIO;
	}

        /* receiving packet buffer allocation - NUM_RX_DESC x MAX_RX_LENGTH */
        for ( i = 0; i < NUM_RX_DESC; i++)
        {
                ei_local->netrx0_skbuf[i] = dev_alloc_skb(MAX_RX_LENGTH + NET_IP_ALIGN);
                if (ei_local->netrx0_skbuf[i] == NULL ) {
                        printk("rx skbuff buffer allocation failed!");
		} else {
		}
        }

	spin_lock_irqsave(&(ei_local->page_lock), flags);
        fe_dma_init(dev);
	fe_sw_init(); //initialize fe and switch register
	err = request_irq( dev->irq, ei_interrupt, 0, dev->name, dev);	// try to fix irq in open
	if (err)
	    return err;

	if ( dev->dev_addr != NULL) {
	    ra2880MacAddressSet((void *)(dev->dev_addr));
	} else {
	    printk("dev->dev_addr is empty !\n");
	} 
        mii_mgr_write(31, 0x7008, 0x1f); //enable switch link change intr
	err = request_irq(31, esw_interrupt, IRQF_DISABLED, "Ralink_ESW", dev);
	if (err)
	    return err;

        sysRegWrite(RAETH_DLY_INT_CFG, DELAY_INT_INIT);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RAETH_DLY_INT_CFG", RAETH_DLY_INT_CFG, DELAY_INT_INIT);
    	sysRegWrite(RAETH_FE_INT_ENABLE, RAETH_FE_INT_DLY_INIT);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "RAETH_FE_INT_ENABLE", RAETH_FE_INT_ENABLE, RAETH_FE_INT_DLY_INIT);

 	INIT_WORK(&ei_local->reset_task, ei_reset_task);

 	INIT_WORK(&ei_local->rx_wq, ei_receive_workq);

	netif_start_queue(dev);


	spin_unlock_irqrestore(&(ei_local->page_lock), flags);


	forward_config(dev);
	return 0;
}

int ei_close(struct net_device *dev)
{
	int i;
	END_DEVICE *ei_local = netdev_priv(dev);        // device pointer
	unsigned long flags;
	spin_lock_irqsave(&(ei_local->page_lock), flags);

	cancel_work_sync(&ei_local->reset_task);
	netif_stop_queue(dev);
	ra2880stop(ei_local);
	msleep(10);

	cancel_work_sync(&ei_local->rx_wq);
	free_irq(dev->irq, dev);
	free_irq(31, dev);
	for ( i = 0; i < NUM_RX_DESC; i++)
	{
		if (ei_local->netrx0_skbuf[i] != NULL) {
			dev_kfree_skb(ei_local->netrx0_skbuf[i]);
			ei_local->netrx0_skbuf[i] = NULL;
		}
	}
	if (ei_local->tx_ring0 != NULL) {
		pci_free_consistent(NULL, NUM_TX_DESC*sizeof(struct PDMA_txdesc), ei_local->tx_ring0, ei_local->phy_tx_ring0);
	}
	pci_free_consistent(NULL, NUM_RX_DESC*sizeof(struct PDMA_rxdesc), ei_local->rx_ring0, ei_local->phy_rx_ring0);

	printk("Free TX/RX Ring Memory!\n");

//	fe_reset();
	spin_unlock_irqrestore(&(ei_local->page_lock), flags);

	module_put(THIS_MODULE);
	return 0;
}


void setup_internal_gsw(void)
{
	u32	i;
	u32	regValue;

	/* reduce RGMII2 PAD driving strength */
	*(volatile u_long *)(PAD_RGMII2_MDIO_CFG) &= ~(0x3 << 4);

	//RGMII1=Normal mode
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) &= ~(0x1 << 14);

	//GMAC1= RGMII mode
	*(volatile u_long *)(SYSCFG1) &= ~(0x3 << 12);

	//enable MDIO to control MT7530
	regValue = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60));
	regValue &= ~(0x3 << 12);
	*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x60) = regValue;

	for(i=0;i<=4;i++)
        {
		//turn off PHY
               mii_mgr_read(i, 0x0 ,&regValue);
	       regValue |= (0x1<<11);
	       mii_mgr_write(i, 0x0, regValue);	
	}
        mii_mgr_write(31, 0x7000, 0x3); //reset switch
        udelay(10);

	if(sysRegRead(0xbe00000c)==0x00030101) {
		sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2005e30b);//(GE1, Force 1000M/FD, FC ON)
		if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RALINK_ETH_SW_BASE+0x100", RALINK_ETH_SW_BASE+0x100, 0x2005e30b);
		mii_mgr_write(31, 0x3600, 0x5e30b);
	} else {
		sysRegWrite(RALINK_ETH_SW_BASE+0x100, 0x2005e33b);//(GE1, Force 1000M/FD, FC ON)
		if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RALINK_ETH_SW_BASE+0x100", RALINK_ETH_SW_BASE+0x100, 0x2005e33b);
		mii_mgr_write(31, 0x3600, 0x5e33b);
	}

	sysRegWrite(RALINK_ETH_SW_BASE+0x200, 0x00008000);//(GE2, Link down)
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RALINK_ETH_SW_BASE+0x200", RALINK_ETH_SW_BASE+0x200, 0x00008000);

	//regValue = 0x117ccf; //Enable Port 6, P5 as GMAC5, P5 disable*/
	mii_mgr_read(31, 0x7804 ,&regValue);
	regValue &= ~(1<<8); //Enable Port 6
	regValue |= (1<<6); //Disable Port 5
	regValue |= (1<<13); //Port 5 as GMAC, no Internal PHY

	regValue |= (1<<16);//change HW-TRAP
	printk("change HW-TRAP to 0x%x!!!!!!!!!!!!",regValue);
	mii_mgr_write(31, 0x7804 ,regValue);
	regValue = *(volatile u_long *)(RALINK_SYSCTL_BASE + 0x10);
	regValue = (regValue >> 6) & 0x7;
	if(regValue >= 6) { //25Mhz Xtal
		/* do nothing */
	} else if(regValue >=3) { //40Mhz

	    mii_mgr_write(0, 13, 0x1f);  // disable MT7530 core clock
	    mii_mgr_write(0, 14, 0x410);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x0);

	    mii_mgr_write(0, 13, 0x1f);  // disable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2020);

	    mii_mgr_write(0, 13, 0x1f);  // for MT7530 core clock = 500Mhz
	    mii_mgr_write(0, 14, 0x40e);  
	    mii_mgr_write(0, 13, 0x401f);  
	    mii_mgr_write(0, 14, 0x119);   

	    mii_mgr_write(0, 13, 0x1f);  // enable MT7530 PLL
	    mii_mgr_write(0, 14, 0x40d);
	    mii_mgr_write(0, 13, 0x401f);
	    mii_mgr_write(0, 14, 0x2820);

	    udelay(20); //suggest by CD

	    mii_mgr_write(0, 13, 0x1f);  // enable MT7530 core clock
	    mii_mgr_write(0, 14, 0x410);
	    mii_mgr_write(0, 13, 0x401f);
	}else { //20Mhz Xtal

		/* TODO */

	}
	mii_mgr_write(0, 14, 0x1);  /*RGMII*/

#if 1
	mii_mgr_write(31, 0x7b00, 0x102);  //delay setting for 10/1000M
	mii_mgr_write(31, 0x7b04, 0x14);  //delay setting for 10/1000M
#else
	mii_mgr_write(31, 0x7b00, 8);  // delay setting for 100M
	mii_mgr_write(31, 0x7b04, 0x14);  // for 100M
#endif
	/*Tx Driving*/
	mii_mgr_write(31, 0x7a54, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a5c, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a64, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a6c, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a74, 0x44);  //lower driving
	mii_mgr_write(31, 0x7a7c, 0x44);  //lower driving

	for(i=0;i<=4;i++)
        {
	//turn on PHY
                mii_mgr_read(i, 0x0 ,&regValue);
	        regValue &= ~(0x1<<11);
	        mii_mgr_write(i, 0x0, regValue);
	}

	mii_mgr_read(31, 0x7808 ,&regValue);
        regValue |= (3<<16); //Enable INTR
	mii_mgr_write(31, 0x7808 ,regValue);
}

int __init ra2882eth_init(void)
{
	int ret;
	struct net_device *dev = alloc_etherdev(sizeof(END_DEVICE));
	if (!dev)
		return -ENOMEM;

	strcpy(dev->name, DEV_NAME);
	dev->irq  = IRQ_ENET0;
	dev->addr_len = 6;
	dev->base_addr = RALINK_FRAME_ENGINE_BASE;

	rather_probe(dev);
	ra2880_setup_dev_fptable(dev);

	if ( register_netdev(dev) != 0) {
		printk(KERN_WARNING " " __FILE__ ": No ethernet port found.\n");
		return -ENXIO;
	}
	ret = 0;

	dev_raether = dev;
	return ret;
}

void fe_sw_init(void)
{
	setup_internal_gsw();
}


void ra2882eth_cleanup_module(void)
{
}
EXPORT_SYMBOL(set_fe_dma_glo_cfg);
module_init(ra2882eth_init);
module_exit(ra2882eth_cleanup_module);
MODULE_LICENSE("GPL");
