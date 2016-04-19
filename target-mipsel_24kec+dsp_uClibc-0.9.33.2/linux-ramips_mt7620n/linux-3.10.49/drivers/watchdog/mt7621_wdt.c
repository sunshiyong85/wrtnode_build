/*
 * Ralink RT288x/RT3xxx/MT76xx built-in hardware watchdog timer
 *
 * Copyright (C) 2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 *
 * This driver was based on: drivers/watchdog/softdog.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/watchdog.h>
#include <linux/miscdevice.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>

#include <asm/mach-ralink/ralink_regs.h>

#define SYSC_RSTSTAT			0x38
#define WDT_RST_CAUSE			BIT(1)

#define RALINK_WDT_TIMEOUT		30

#define TIMER_REG_TMRSTAT		0x00
#define TIMER_REG_TMR1LOAD		0x24
#define TIMER_REG_TMR1CTL		0x20

#define TMR1CTL_ENABLE			BIT(7)
#define TMR1CTL_RESTART			BIT(9)

static void __iomem *mt762x_wdt_base;

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		"Watchdog cannot be stopped once started (default="
		__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static inline void rt_wdt_w32(unsigned reg, u32 val)
{
	iowrite32(val, mt762x_wdt_base + reg);
}

static inline u32 rt_wdt_r32(unsigned reg)
{
	return ioread32(mt762x_wdt_base + reg);
}

static int mt762x_wdt_ping(struct watchdog_device *w)
{
	rt_wdt_w32(TIMER_REG_TMRSTAT, TMR1CTL_RESTART);

	return 0;
}

static int mt762x_wdt_set_timeout(struct watchdog_device *w, unsigned int t)
{
	w->timeout = t;
	rt_wdt_w32(TIMER_REG_TMR1LOAD, t * 1000);
	mt762x_wdt_ping(w);

	return 0;
}

static int mt762x_wdt_start(struct watchdog_device *w)
{
	u32 t;

	rt_wdt_w32(TIMER_REG_TMR1CTL, 1000 << 16);
	mt762x_wdt_set_timeout(w, w->timeout);

	t = rt_wdt_r32(TIMER_REG_TMR1CTL);
	t |= TMR1CTL_ENABLE;
	rt_wdt_w32(TIMER_REG_TMR1CTL, t);

	return 0;
}

static int mt762x_wdt_stop(struct watchdog_device *w)
{
	u32 t;

	mt762x_wdt_ping(w);

	t = rt_wdt_r32(TIMER_REG_TMR1CTL);
	t &= ~TMR1CTL_ENABLE;
	rt_wdt_w32(TIMER_REG_TMR1CTL, t);

	return 0;
}

static int mt762x_wdt_bootcause(void)
{
	if (rt_sysc_r32(SYSC_RSTSTAT) & WDT_RST_CAUSE)
		return WDIOF_CARDRESET;

	return 0;
}

static struct watchdog_info mt762x_wdt_info = {
	.identity = "Mediatek Watchdog",
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static struct watchdog_ops mt762x_wdt_ops = {
	.owner = THIS_MODULE,
	.start = mt762x_wdt_start,
	.stop = mt762x_wdt_stop,
	.ping = mt762x_wdt_ping,
	.set_timeout = mt762x_wdt_set_timeout,
};

static struct watchdog_device mt762x_wdt_dev = {
	.info = &mt762x_wdt_info,
	.ops = &mt762x_wdt_ops,
	.min_timeout = 1,
};

static int mt762x_wdt_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mt762x_wdt_base = devm_request_and_ioremap(&pdev->dev, res);
	if (IS_ERR(mt762x_wdt_base))
		return PTR_ERR(mt762x_wdt_base);

	device_reset(&pdev->dev);

	mt762x_wdt_dev.dev = &pdev->dev;
	mt762x_wdt_dev.bootstatus = mt762x_wdt_bootcause();
	mt762x_wdt_dev.max_timeout = (0xfffful / 1000);
	mt762x_wdt_dev.timeout = mt762x_wdt_dev.max_timeout;

	watchdog_set_nowayout(&mt762x_wdt_dev, nowayout);

	ret = watchdog_register_device(&mt762x_wdt_dev);
	if (!ret)
		dev_info(&pdev->dev, "Initialized\n");

	return 0;
}

static int mt762x_wdt_remove(struct platform_device *pdev)
{
	watchdog_unregister_device(&mt762x_wdt_dev);

	return 0;
}

static void mt762x_wdt_shutdown(struct platform_device *pdev)
{
	mt762x_wdt_stop(&mt762x_wdt_dev);
}

static const struct of_device_id mt762x_wdt_match[] = {
	{ .compatible = "mtk,mt7621-wdt" },
	{},
};
MODULE_DEVICE_TABLE(of, mt762x_wdt_match);

static struct platform_driver mt762x_wdt_driver = {
	.probe		= mt762x_wdt_probe,
	.remove		= mt762x_wdt_remove,
	.shutdown	= mt762x_wdt_shutdown,
	.driver		= {
		.name		= KBUILD_MODNAME,
		.owner		= THIS_MODULE,
		.of_match_table	= mt762x_wdt_match,
	},
};

module_platform_driver(mt762x_wdt_driver);

MODULE_DESCRIPTION("MediaTek MT762x hardware watchdog driver");
MODULE_AUTHOR("John Crispin <blogic@openwrt.org");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
