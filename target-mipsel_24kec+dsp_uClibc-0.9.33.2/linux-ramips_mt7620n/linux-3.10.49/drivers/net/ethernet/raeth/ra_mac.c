#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/irq.h>
#include <linux/ctype.h>

#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/dma.h>

#include <asm/rt2880/surfboardint.h>	/* for cp0 reg access, added by bobtseng */

#include <linux/errno.h>
#include <linux/init.h>
//#include <linux/mca.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#if defined(CONFIG_USER_SNMPD)
#include <linux/seq_file.h>
#endif



#include "ra2882ethreg.h"
#include "raether.h"
#include "ra_mac.h"

extern struct net_device *dev_raether;


void ra2880stop(END_DEVICE *ei_local)
{
	unsigned int regValue;
	printk("ra2880stop()...");

	regValue = sysRegRead(PDMA_GLO_CFG);
	regValue &= ~(TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN);
	sysRegWrite(PDMA_GLO_CFG, regValue);
	printk("-> %s 0x%08x 0x%08x\n", "PDMA_GLO_CFG", PDMA_GLO_CFG, regValue);
	printk("Done\n");
}

void ei_irq_clear(void)
{
        sysRegWrite(FE_INT_STATUS, 0xFFFFFFFF);
	printk("-> %s 0x%08x 0x%08x\n", "FE_INT_STATUS", FE_INT_STATUS, 0xFFFFFFFF);
}

void rt2880_gmac_hard_reset(void)
{
	sysRegWrite(RSTCTRL, RALINK_FE_RST);
	printk("-> %s 0x%08x 0x%08x\n", "RSTCTRL", RSTCTRL, RALINK_FE_RST);
	sysRegWrite(RSTCTRL, 0);
	printk("-> %s 0x%08x 0x%08x\n", "RSTCTRL", RSTCTRL, 0);
}

void ra2880EnableInterrupt()
{
	unsigned int regValue = sysRegRead(FE_INT_ENABLE);
	sysRegWrite(FE_INT_ENABLE, regValue);
	printk("-> %s 0x%08x 0x%08x\n", "FE_INT_ENABLE", FE_INT_ENABLE, regValue);
}

void ra2880MacAddressSet(unsigned char p[6])
{
        unsigned long regValue;

	regValue = (p[0] << 8) | (p[1]);
        sysRegWrite(GDMA1_MAC_ADRH, regValue);
	printk("-> %s 0x%08x 0x%08x\n", "GDMA1_MAC_ADRH", GDMA1_MAC_ADRH, regValue);

        regValue = (p[2] << 24) | (p[3] <<16) | (p[4] << 8) | p[5];
	printk("-> %s 0x%08x 0x%08x\n", "GDMA1_MAC_ADRL", GDMA1_MAC_ADRL, regValue);
        sysRegWrite(GDMA1_MAC_ADRL, regValue);

        return;
}


