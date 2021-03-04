// SPDX-License-Identifier: GPL-2.0
/*
 * loongson-specific suspend support
 *
 *  Copyright (C) 2009 - 2012 Lemote Inc.
 *  Author: Wu Zhangjin <wuzhangjin@gmail.com>
 *          Huacai Chen <chenhc@lemote.com>
 * Copyright (C) 2020 Loongson Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/pm.h>

#include <loongson-pch.h>
#include <asm/loongarchregs.h>

#include <loongson.h>

#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>

#include <asm/loongarchregs.h>
#include <asm/bootinfo.h>
#include <asm/tlbflush.h>

#include <loongson.h>
#include <mc146818rtc.h>
#include <loongson-pch.h>
#include <lcsrintrin.h>

extern void acpi_sleep_prepare(void);
extern void acpi_sleep_complete(void);
extern void acpi_registers_setup(void);

u32 loongson_nr_nodes;
u64 loongson_suspend_addr;
u32 loongson_pcache_ways;
u32 loongson_scache_ways;
u32 loongson_pcache_sets;
u32 loongson_scache_sets;
u32 loongson_pcache_linesz;
u32 loongson_scache_linesz;

struct loongson_registers {
	u32 config4;
	u32 config6;
	u64 pgd;
	u64 kpgd;
	u32 pwctl0;
	u32 pwctl1;
	u64 ebase;
};
static struct loongson_registers loongson_regs;

void mach_common_suspend(void)
{
	if (cpu_has_ldpte) {
		loongson_regs.config4 = __lcsr_dcsrrd(LOONGARCH_CSR_EXCFG);
		loongson_regs.pgd = __lcsr_dcsrrd(LOONGARCH_CSR_PGDL);
		loongson_regs.kpgd = __lcsr_dcsrrd(LOONGARCH_CSR_PGDH);
		loongson_regs.pwctl0 = __lcsr_dcsrrd(LOONGARCH_CSR_PWCTL0);
		loongson_regs.pwctl1 = __lcsr_dcsrrd(LOONGARCH_CSR_PWCTL1);
	}

	if (cpu_has_divec)
		loongson_regs.ebase = __lcsr_dcsrrd(LOONGARCH_CSR_EBASE);

	loongson_nr_nodes = loongson_sysconf.nr_nodes;
	loongson_suspend_addr = loongson_sysconf.suspend_addr;

	loongson_pcache_ways = cpu_data[0].dcache.ways;
	loongson_scache_ways = cpu_data[0].scache.ways;
	loongson_pcache_sets = cpu_data[0].dcache.sets;
	loongson_scache_sets = cpu_data[0].scache.sets;
	loongson_pcache_linesz = cpu_data[0].dcache.linesz;
	loongson_scache_linesz = cpu_data[0].scache.linesz;
}

void mach_common_resume(void)
{
	local_flush_tlb_all();

	if (cpu_has_ldpte) {
		__lcsr_dcsrwr(loongson_regs.config4, LOONGARCH_CSR_EXCFG);
		__lcsr_dcsrwr(loongson_regs.pgd, LOONGARCH_CSR_PGDL);
		__lcsr_dcsrwr(loongson_regs.kpgd, LOONGARCH_CSR_PGDH);
		__lcsr_dcsrwr(loongson_regs.pwctl0, LOONGARCH_CSR_PWCTL0);
		__lcsr_dcsrwr(loongson_regs.pwctl1, LOONGARCH_CSR_PWCTL1);
	}

	if (cpu_has_divec)
		__lcsr_dcsrwr(loongson_regs.ebase, LOONGARCH_CSR_EBASE);
}

void mach_suspend(void)
{
	acpi_sleep_prepare();
	mach_common_suspend();
}

void mach_resume(void)
{
	mach_common_resume();
	acpi_registers_setup();
	acpi_sleep_complete();
}
void arch_suspend_disable_irqs(void)
{
	/* disable all loongarch events */
	local_irq_disable();

	LOONGSON_INTENCLR = 0xffff;
	(void)LOONGSON_INTENCLR;
}

void arch_suspend_enable_irqs(void)
{
	/* enable all loongarch events */
	local_irq_enable();
	(void)LOONGSON_INTENSET;
}

static int loongson_pm_begin(suspend_state_t state)
{
	pm_set_suspend_via_firmware();
	return 0;
}

static void loongson_pm_wake(void)
{
#ifdef CONFIG_CPU_LOONGSON3
	disable_unused_cpus();
#endif
}

static void loongson_pm_end(void)
{
}

static int loongson_pm_enter(suspend_state_t state)
{
	mach_suspend();

	/* processor specific suspend */
	loongson_suspend_enter();
	pm_set_resume_via_firmware();

	mach_resume();

	return 0;
}

static int loongson_pm_valid_state(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_ON:
		return 1;

	case PM_SUSPEND_MEM:
#ifndef CONFIG_CPU_LOONGSON3
		return 1;
#else
		return !!loongson_sysconf.suspend_addr;
#endif

	default:
		return 0;
	}
}

static const struct platform_suspend_ops loongson_pm_ops = {
	.valid	= loongson_pm_valid_state,
	.begin	= loongson_pm_begin,
	.enter	= loongson_pm_enter,
	.wake	= loongson_pm_wake,
	.end	= loongson_pm_end,
};

static int __init loongson_pm_init(void)
{
	suspend_set_ops(&loongson_pm_ops);

	return 0;
}
arch_initcall(loongson_pm_init);
