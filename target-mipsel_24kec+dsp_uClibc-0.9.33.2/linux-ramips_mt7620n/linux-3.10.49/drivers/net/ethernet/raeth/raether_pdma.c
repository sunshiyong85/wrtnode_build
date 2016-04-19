#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
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

#define	MAX_RX_LENGTH	1536

extern int reg_dbg;
extern struct net_device		*dev_raether;
static unsigned long tx_ring_full=0;

#define KSEG1                   0xa0000000
#define PHYS_TO_VIRT(x)         ((void *)((x) | KSEG1))
#define VIRT_TO_PHYS(x)         ((unsigned long)(x) & ~KSEG1)

extern void set_fe_dma_glo_cfg(void);

int fe_dma_init(struct net_device *dev)
{

	int		i;
	unsigned int	regVal;
	END_DEVICE* ei_local = netdev_priv(dev);

	while(1)
	{
		regVal = sysRegRead(PDMA_GLO_CFG);
		if((regVal & RX_DMA_BUSY))
		{
			printk("\n  RX_DMA_BUSY !!! ");
			continue;
		}
		if((regVal & TX_DMA_BUSY))
		{
			printk("\n  TX_DMA_BUSY !!! ");
			continue;
		}
		break;
	}

	for (i=0;i<NUM_TX_DESC;i++){
		ei_local->skb_free[i]=0;
	}
	ei_local->free_idx =0;
	ei_local->tx_ring0 = pci_alloc_consistent(NULL, NUM_TX_DESC * sizeof(struct PDMA_txdesc), &ei_local->phy_tx_ring0);
	printk("\nphy_tx_ring = 0x%08x, tx_ring = 0x%p\n", ei_local->phy_tx_ring0, ei_local->tx_ring0);

	for (i=0; i < NUM_TX_DESC; i++) {
		memset(&ei_local->tx_ring0[i],0,sizeof(struct PDMA_txdesc));
		ei_local->tx_ring0[i].txd_info2.LS0_bit = 1;
		ei_local->tx_ring0[i].txd_info2.DDONE_bit = 1;

	}

	/* Initial RX Ring 0*/
	ei_local->rx_ring0 = pci_alloc_consistent(NULL, NUM_RX_DESC * sizeof(struct PDMA_rxdesc), &ei_local->phy_rx_ring0);
	for (i = 0; i < NUM_RX_DESC; i++) {
		memset(&ei_local->rx_ring0[i],0,sizeof(struct PDMA_rxdesc));
		ei_local->rx_ring0[i].rxd_info2.DDONE_bit = 0;
		ei_local->rx_ring0[i].rxd_info2.LS0 = 0;
		ei_local->rx_ring0[i].rxd_info2.PLEN0 = MAX_RX_LENGTH;
		ei_local->rx_ring0[i].rxd_info1.PDP0 = dma_map_single(NULL, ei_local->netrx0_skbuf[i]->data, MAX_RX_LENGTH, PCI_DMA_FROMDEVICE);
	}
	printk("\nphy_rx_ring0 = 0x%08x, rx_ring0 = 0x%p\n",ei_local->phy_rx_ring0,ei_local->rx_ring0);


	regVal = sysRegRead(PDMA_GLO_CFG);
	regVal &= 0x000000FF;
	sysRegWrite(PDMA_GLO_CFG, regVal);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "PDMA_GLO_CFG", PDMA_GLO_CFG, regVal);

	regVal=sysRegRead(PDMA_GLO_CFG);

	/* Tell the adapter where the TX/RX rings are located. */
        sysRegWrite(TX_BASE_PTR0, phys_to_bus((u32) ei_local->phy_tx_ring0));
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "TX_BASE_PTR0", TX_BASE_PTR0, phys_to_bus((u32) ei_local->phy_tx_ring0));
	sysRegWrite(TX_MAX_CNT0, cpu_to_le32((u32) NUM_TX_DESC));
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "TX_MAX_CNT0", TX_MAX_CNT0, cpu_to_le32((u32) NUM_TX_DESC));
	sysRegWrite(TX_CTX_IDX0, 0);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "TX_CTX_IDX0", TX_CTX_IDX0, 0);
	sysRegWrite(PDMA_RST_CFG, PST_DTX_IDX0);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "PDMA_RST_CFG", PDMA_RST_CFG, PST_DTX_IDX0);

	sysRegWrite(RX_BASE_PTR0, phys_to_bus((u32) ei_local->phy_rx_ring0));
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RX_BASE_PTR0", RX_BASE_PTR0, phys_to_bus((u32) ei_local->phy_rx_ring0));
	sysRegWrite(RX_MAX_CNT0,  cpu_to_le32((u32) NUM_RX_DESC));
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RX_MAX_CNT0", RX_MAX_CNT0, cpu_to_le32((u32) NUM_RX_DESC));
	sysRegWrite(RX_CALC_IDX0, cpu_to_le32((u32) (NUM_RX_DESC - 1)));
	if (reg_dbg) printk("-> %s 0x%08x 0x%08x\n", "RX_CALC_IDX0", RX_CALC_IDX0, cpu_to_le32((u32) (NUM_RX_DESC - 1)));
	sysRegWrite(PDMA_RST_CFG, PST_DRX_IDX0);
	if (reg_dbg) printk("-> %s 0x%08x 0x%08lx\n", "PDMA_RST_CFG", PDMA_RST_CFG, PST_DRX_IDX0);

	set_fe_dma_glo_cfg();

	return 1;
}

inline int rt2880_eth_send(struct net_device* dev, struct sk_buff *skb, int gmac_no)
{
	unsigned int	length=skb->len;
	END_DEVICE*	ei_local = netdev_priv(dev);
	unsigned long	tx_cpu_owner_idx0 = sysRegRead(TX_CTX_IDX0);

	while(ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit == 0)
	{
			ei_local->stat.tx_errors++;
	}

	ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info1.SDP0 = virt_to_phys(skb->data);
	ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.SDL0 = length;
	if (gmac_no == 1) {
		ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info4.FPORT = 1;
	}else {
		ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info4.FPORT = 2;
	}
	ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit = 0;
	tx_cpu_owner_idx0 = (tx_cpu_owner_idx0+1) % NUM_TX_DESC;
	while(ei_local->tx_ring0[tx_cpu_owner_idx0].txd_info2.DDONE_bit == 0)
	{
		ei_local->stat.tx_errors++;
	}
	sysRegWrite(TX_CTX_IDX0, cpu_to_le32((u32)tx_cpu_owner_idx0));

	{
		ei_local->stat.tx_packets++;
		ei_local->stat.tx_bytes += length;
	}

	return length;
}

int ei_start_xmit(struct sk_buff* skb, struct net_device *dev, int gmac_no)
{
	END_DEVICE *ei_local = netdev_priv(dev);
	unsigned long flags;
	unsigned long tx_cpu_owner_idx;
	unsigned int tx_cpu_owner_idx_next;
	unsigned int num_of_txd;
	unsigned int tx_cpu_owner_idx_next2;

	dev->trans_start = jiffies;	/* save the timestamp */
	spin_lock_irqsave(&ei_local->page_lock, flags);
	dma_cache_sync(NULL, skb->data, skb->len, DMA_TO_DEVICE);

	tx_cpu_owner_idx = sysRegRead(TX_CTX_IDX0);
	num_of_txd = 1;
	tx_cpu_owner_idx_next = (tx_cpu_owner_idx + num_of_txd) % NUM_TX_DESC;

	if(((ei_local->skb_free[tx_cpu_owner_idx]) ==0) && (ei_local->skb_free[tx_cpu_owner_idx_next]==0)){
		rt2880_eth_send(dev, skb, gmac_no);

		tx_cpu_owner_idx_next2 = (tx_cpu_owner_idx_next + 1) % NUM_TX_DESC;

		if(ei_local->skb_free[tx_cpu_owner_idx_next2]!=0){
		}
	}else {
			ei_local->stat.tx_dropped++;
		kfree_skb(skb);
		spin_unlock_irqrestore(&ei_local->page_lock, flags);
		return 0;
	}

	ei_local->skb_free[tx_cpu_owner_idx] = skb;
	spin_unlock_irqrestore(&ei_local->page_lock, flags);
	return 0;
}

void ei_xmit_housekeeping(unsigned long unused)
{
    struct net_device *dev = dev_raether;
    END_DEVICE *ei_local = netdev_priv(dev);
    struct PDMA_txdesc *tx_desc;
    unsigned long skb_free_idx;
    unsigned long tx_dtx_idx __maybe_unused;
    unsigned long reg_int_mask=0;

	tx_dtx_idx = sysRegRead(TX_DTX_IDX0);
	tx_desc = ei_local->tx_ring0;
	skb_free_idx = ei_local->free_idx;
	if ((ei_local->skb_free[skb_free_idx]) != 0 && tx_desc[skb_free_idx].txd_info2.DDONE_bit==1) {
		while(tx_desc[skb_free_idx].txd_info2.DDONE_bit==1 && (ei_local->skb_free[skb_free_idx])!=0 ){
	    dev_kfree_skb_any(ei_local->skb_free[skb_free_idx]);
	    ei_local->skb_free[skb_free_idx]=0;
	    skb_free_idx = (skb_free_idx +1) % NUM_TX_DESC;
	}

	netif_wake_queue(dev);
		tx_ring_full=0;
		ei_local->free_idx = skb_free_idx;
	}

    reg_int_mask=sysRegRead(FE_INT_ENABLE);
    sysRegWrite(FE_INT_ENABLE, reg_int_mask| TX_DLY_INT);
}

EXPORT_SYMBOL(ei_start_xmit);
EXPORT_SYMBOL(ei_xmit_housekeeping);
EXPORT_SYMBOL(fe_dma_init);
EXPORT_SYMBOL(rt2880_eth_send);
