// SPDX-License-Identifier: GPL-2.0
/*
 * Loongson-2H Real Time Clock interface for Linux
 *
 * Author: Shaozong Liu <liushaozong@loongson.cn>
 *	   Huacai Chen <chenhc@lemote.com>
 *
 */

#include <linux/acpi.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/time.h>

#ifdef CONFIG_CPU_LOONGSON2K
#include <loongson-2k.h>
#else
#include <loongson-pch.h>
#endif

/**
 * Loongson-2H rtc register
 */

#define TOY_TRIM_REG   0x20
#define TOY_WRITE0_REG 0x24
#define TOY_WRITE1_REG 0x28
#define TOY_READ0_REG  0x2c
#define TOY_READ1_REG  0x30
#define TOY_MATCH0_REG 0x34
#define TOY_MATCH1_REG 0x38
#define TOY_MATCH2_REG 0x3c
#define RTC_CTRL_REG   0x40
#define RTC_TRIM_REG   0x60
#define RTC_WRITE0_REG 0x64
#define RTC_READE0_REG 0x68
#define RTC_MATCH0_REG 0x6c
#define RTC_MATCH1_REG 0x70
#define RTC_MATCH2_REG 0x74

/**
 * shift bits and filed mask
 */
#define TOY_MON_MASK   0x3f
#define TOY_DAY_MASK   0x1f
#define TOY_HOUR_MASK  0x1f
#define TOY_MIN_MASK   0x3f
#define TOY_SEC_MASK   0x3f
#define TOY_MSEC_MASK  0xf

#define TOY_MON_SHIFT  26
#define TOY_DAY_SHIFT  21
#define TOY_HOUR_SHIFT 16
#define TOY_MIN_SHIFT  10
#define TOY_SEC_SHIFT  4
#define TOY_MSEC_SHIFT 0

/* shift bits for TOY_MATCH */
#define TOY_MATCH_YEAR_SHIFT 26
#define TOY_MATCH_MON_SHIFT  22
#define TOY_MATCH_DAY_SHIFT  17
#define TOY_MATCH_HOUR_SHIFT 12
#define TOY_MATCH_MIN_SHIFT  6
#define TOY_MATCH_SEC_SHIFT  0

/* Filed mask bits for TOY_MATCH */
#define TOY_MATCH_YEAR_MASK  0x3f
#define TOY_MATCH_MON_MASK   0xf
#define TOY_MATCH_DAY_MASK   0x1f
#define TOY_MATCH_HOUR_MASK  0x1f
#define TOY_MATCH_MIN_MASK   0x3f
#define TOY_MATCH_SEC_MASK   0x3f

#define LS7A_REVISION_R1	0x0080
#define LS7A_REVISION_R2	0x0181

/* ACPI and RTC offset */
#ifdef CONFIG_CPU_LOONGSON2K
#define ACPI_RTC_OFFSET		0x800
#else
#define ACPI_RTC_OFFSET		0x100
#endif

/* support rtc wakeup */
#define PM1_STS_FOR_RTC		0x10
#define PM1_STS_STATUS_REG	0x0c
#define RTC_STS_WAKEUP_BIT	(0x1 << 10)
#define RTC_STS_STATUS_BIT	(0x1 << 10)
#define ACPI_FOR_RTC_WAKEUP_BASE	\
	(rtc_reg_base - ACPI_RTC_OFFSET + PM1_STS_FOR_RTC)
#define ACPI_FOR_RTC_STATUS_BASE	\
	(rtc_reg_base - ACPI_RTC_OFFSET + PM1_STS_STATUS_REG)


/* interface for rtc read and write */
#define rtc_write(val, addr)   writel(val, rtc_reg_base + (addr))
#define rtc_read(addr)         readl(rtc_reg_base + (addr))

struct ls2x_rtc_info {
	struct platform_device *pdev;
	struct rtc_device *rtc_dev;
	struct resource *mem_res;
	void __iomem *rtc_base;
	int irq_base;
#ifdef CONFIG_CPU_LOONGSON2K
	unsigned long acpi_pm1_sts;
#endif
};

static void __iomem *rtc_reg_base;

static int ls2x_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int val;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);

	val = rtc_read(TOY_READ1_REG);
	tm->tm_year = val;
	val = rtc_read(TOY_READ0_REG);
	tm->tm_sec = (val >> TOY_SEC_SHIFT) & TOY_SEC_MASK;
	tm->tm_min = (val >> TOY_MIN_SHIFT) & TOY_MIN_MASK;
	tm->tm_hour = (val >> TOY_HOUR_SHIFT) & TOY_HOUR_MASK;
	tm->tm_mday = (val >> TOY_DAY_SHIFT) & TOY_DAY_MASK;
	tm->tm_mon = ((val >> TOY_MON_SHIFT) & TOY_MON_MASK) - 1;

	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static int ls2x_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int val = 0;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);

	val |= (tm->tm_sec << TOY_SEC_SHIFT);
	val |= (tm->tm_min << TOY_MIN_SHIFT);
	val |= (tm->tm_hour << TOY_HOUR_SHIFT);
	val |= (tm->tm_mday << TOY_DAY_SHIFT);
	val |= ((tm->tm_mon + 1) << TOY_MON_SHIFT);
	rtc_write(val, TOY_WRITE0_REG);
	val = tm->tm_year;
	rtc_write(val, TOY_WRITE1_REG);

	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static int ls2x_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned int val;
	unsigned long flags;
	struct rtc_time *tm = &alrm->time;

	spin_lock_irqsave(&rtc_lock, flags);

	val = rtc_read(TOY_MATCH0_REG);
	tm->tm_sec = (val >> TOY_MATCH_SEC_SHIFT) & TOY_MATCH_SEC_MASK;
	tm->tm_min = (val >> TOY_MATCH_MIN_SHIFT) & TOY_MATCH_MIN_MASK;
	tm->tm_hour = (val >> TOY_MATCH_HOUR_SHIFT) & TOY_MATCH_HOUR_MASK;
	tm->tm_mday = (val >> TOY_MATCH_DAY_SHIFT) & TOY_MATCH_DAY_MASK;
	tm->tm_mon = ((val >> TOY_MATCH_MON_SHIFT) & TOY_MATCH_MON_MASK) - 1;
	tm->tm_year = ((val >> TOY_MATCH_YEAR_SHIFT) & TOY_MATCH_YEAR_MASK);

	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static int ls2x_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned int val = 0;
	unsigned int temp = 0;
	unsigned long flags;
	struct rtc_time *tm = &alrm->time;

	spin_lock_irqsave(&rtc_lock, flags);

	val |= (tm->tm_sec << TOY_MATCH_SEC_SHIFT);
	val |= (tm->tm_min << TOY_MATCH_MIN_SHIFT);
	val |= (tm->tm_hour << TOY_MATCH_HOUR_SHIFT);
	val |= (tm->tm_mday << TOY_MATCH_DAY_SHIFT);
	val |= ((tm->tm_mon + 1) << TOY_MATCH_MON_SHIFT);
	val |= ((tm->tm_year & TOY_MATCH_YEAR_MASK) << TOY_MATCH_YEAR_SHIFT);
	rtc_write(val, TOY_MATCH0_REG);

	/* enalbe acpi rtc wakeup */
	temp = readl(ACPI_FOR_RTC_WAKEUP_BASE) | RTC_STS_WAKEUP_BIT;
	writel(temp, ACPI_FOR_RTC_WAKEUP_BASE);

	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static struct rtc_class_ops ls2x_rtc_ops = {
	.read_time = ls2x_rtc_read_time,
	.set_time = ls2x_rtc_set_time,
	.read_alarm = ls2x_rtc_read_alarm,
	.set_alarm = ls2x_rtc_set_alarm,
};


static int ls2x_rtc_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct resource *res, *mem;
	struct rtc_device *rtc;
	struct ls2x_rtc_info *info;
	unsigned int __maybe_unused revision;

	info = kzalloc(sizeof(struct ls2x_rtc_info), GFP_KERNEL);
	if (!info) {
		pr_debug("%s: no enough memory\n", pdev->name);
		return -ENOMEM;
	}

	info->pdev = pdev;
	info->irq_base = platform_get_irq(pdev, 0);
	if (info->irq_base <= 0) {
		pr_debug("%s: no irq?\n", pdev->name);
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_debug("%s: RTC resource data missing\n", pdev->name);
		return -ENOENT;
	}

	mem = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!mem) {
		pr_debug("%s: RTC registers at %x are not free\n",
			 pdev->name, (unsigned int)res->start);
		return -EBUSY;
	}
	info->mem_res = mem;

	info->rtc_base = ioremap(res->start, resource_size(res));
	if (!info->rtc_base) {
		pr_debug("%s: RTC registers can't be mapped\n", pdev->name);
		goto fail1;
	}
	rtc_reg_base = info->rtc_base;

	ret = device_init_wakeup(dev, 1);

	rtc = info->rtc_dev = rtc_device_register(pdev->name, &pdev->dev,
						  &ls2x_rtc_ops, THIS_MODULE);

#ifdef CONFIG_CPU_LOONGSON3
	/* get 7A pch revision number */
	revision = readl((void *)TO_UNCAC(LS7A_CHIPCFG_REG_BASE + 0x3ffc)) >> 16U;
	if (revision == LS7A_REVISION_R1 || revision == LS7A_REVISION_R2) {
		rtc->set_offset_nsec = -900000000;
	} else {
		rtc->set_offset_nsec = 0;
	}
#endif

	/* There don't need alarm interrupt */
	info->rtc_dev->uie_unsupported = 1;

	if (IS_ERR(info->rtc_dev)) {
		pr_debug("%s: can't register RTC device, err %ld\n",
			 pdev->name, PTR_ERR(rtc));
		goto fail0;
	}

	platform_set_drvdata(pdev, info);
	dev_set_drvdata(&rtc->dev, info);


	if (ret)
		dev_warn(dev, "init wakeup fail, ERRNO is %d\n", ret);
	return ret;

fail0:
	iounmap(info->rtc_base);
fail1:
	release_resource(mem);
	kfree(info);

	return -EIO;
}

static int ls2x_rtc_remove(struct platform_device *pdev)
{
	struct ls2x_rtc_info *info = platform_get_drvdata(pdev);
	struct rtc_device *rtc = info->rtc_dev;

	iounmap(info->rtc_base);
	release_resource(dev_get_drvdata(&rtc->dev));
	release_resource(info->mem_res);
	rtc_device_unregister(rtc);
	kfree(info);

	return 0;
}

static int ls2x_rtc_suspend(struct device *dev)
{
#ifdef CONFIG_CPU_LOONGSON2K
	unsigned long flags;
	struct ls2x_rtc_info *info = dev_get_drvdata(dev);

	spin_lock_irqsave(&rtc_lock, flags);

	info->acpi_pm1_sts = readl(ACPI_FOR_RTC_STATUS_BASE);

	spin_unlock_irqrestore(&rtc_lock, flags);
#endif
	return 0;
}

static int ls2x_rtc_resume(struct device *dev)
{
#ifdef CONFIG_CPU_LOONGSON2K
	unsigned long flags;
	unsigned long temp;
	struct ls2x_rtc_info *info = dev_get_drvdata(dev);

	spin_lock_irqsave(&rtc_lock, flags);

	/* clear acpi RTC_STS (RTC Status) */
	temp = info->acpi_pm1_sts | RTC_STS_STATUS_BIT;

	writel(temp, ACPI_FOR_RTC_STATUS_BASE);

	spin_unlock_irqrestore(&rtc_lock, flags);

	dev_dbg(dev, "resume, acpi PM1_STS: %lx\n", temp);
#endif
	return 0;
}

static SIMPLE_DEV_PM_OPS(ls2x_rtc_pm_ops, ls2x_rtc_suspend, ls2x_rtc_resume);

#ifdef CONFIG_OF
static struct of_device_id ls2x_rtc_id_table[] = {
	{.compatible = "loongson,ls2h-rtc"},
	{.compatible = "loongson,ls2k-rtc"},
	{.compatible = "loongson,ls7a-rtc"},
	{.compatible = "loongson,ls-rtc"},
	{},
};
#endif

static const struct acpi_device_id loongson_rtc_acpi_match[] = {
	{"LOON0001"},
	{}
};
MODULE_DEVICE_TABLE(acpi, loongson_rtc_acpi_match);

static struct platform_driver ls2x_rtc_driver = {
	.probe		= ls2x_rtc_probe,
	.remove		= ls2x_rtc_remove,
	.driver		= {
		.name	= "ls2x-rtc",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ls2x_rtc_id_table),
#endif
		.acpi_match_table = ACPI_PTR(loongson_rtc_acpi_match),
		.pm = &ls2x_rtc_pm_ops,
	},
};

static int __init rtc_init(void)
{
	return platform_driver_register(&ls2x_rtc_driver);
}

static void __exit rtc_exit(void)
{
	platform_driver_unregister(&ls2x_rtc_driver);
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_AUTHOR("Liu Shaozong");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ls2x-rtc");
