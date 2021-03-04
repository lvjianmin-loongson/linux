// SPDX-License-Identifier: GPL-2.0
/*
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  Copyright (C) 2020 Loongson Technology Corporation Limited
 */
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/stddef.h>
#include <irq.h>
#include <lcsrintrin.h>
#include <linux/module.h>
#include <linux/syscore_ops.h>
#include <asm/irq_cpu.h>
#include <asm/setup.h>
#include <asm/loongarchregs.h>
#include <loongson.h>
#include <loongson-pch.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <boot_param.h>

static int nr_pch_pics;
#define	for_each_pch_pic(idx)		\
	for ((idx) = 0; (idx) < nr_pch_pics; (idx)++)
#define MSI_IRQ_NR_64           64
#define MSI_IRQ_NR_192          192
#define MSI_IRQ_NR_224          224
#define LIOINTC_MEM_SIZE	0x80
#define LIOINTC_VECS_TO_IP2	0x00FFFFFE /* others */
#define LIOINTC_VECS_TO_IP3	0xFF000000 /* HT1 0-7 */
#define PCH_PIC_SIZE		0x400
extern const struct plat_smp_ops *mp_ops;
u64 liointc_base = LIOINTC_DEFAULT_PHYS_BASE;
static int msi_irqbase;

static enum pch_irq_route_model_id pch_irq_route_model = PCH_IRQ_ROUTE_EXT;
enum pch_irq_route_model_id get_irq_route_model(void)
{
	return pch_irq_route_model;
}

static struct pch_pic {
	/*
	 * # of IRQ routing registers
	 */
	int nr_registers;

	/* pch pic config */
	struct pch_pic_config config;
	/* pch pic irq routing info */
	struct pch_pic_irq  irq_config;
} pch_pics[MAX_PCH_PICS];

struct pch_pic_irq *pch_pic_irq_routing(int pch_pic_idx)
{
	return &pch_pics[pch_pic_idx].irq_config;
}

int pch_pic_id(int pch_pic_idx)
{
	return pch_pics[pch_pic_idx].config.pch_pic_id;
}

unsigned int pch_pic_addr(int pch_pic_idx)
{
	return pch_pics[pch_pic_idx].config.pch_pic_addr;
}

static __init int bad_pch_pic(unsigned long address)
{
	if (nr_pch_pics >= MAX_PCH_PICS) {
		pr_warn("WARNING: Max # of I/O PCH_PICs (%d) exceeded (found %d), skipping\n",
			MAX_PCH_PICS, nr_pch_pics);
		return 1;
	}
	if (!address) {
		pr_warn("WARNING: Bogus (zero) I/O PCH_PIC address found in table, skipping!\n");
		return 1;
	}
	return 0;
}
#define pch_pic_ver(pch_pic_idx)	pch_pics[pch_pic_idx].config.pch_pic_ver
void __init register_pch_pic(int id, u32 address, u32 irq_base)
{
	int idx = 0;
	int entries;
	struct pch_pic_irq *irq_cfg;

	if (bad_pch_pic(address))
		return;

	idx = nr_pch_pics;

	pch_pics[idx].config.pch_pic_addr = address;

	pch_pics[idx].config.pch_pic_id = id;
	pch_pics[idx].config.pch_pic_ver = 0;

	/*
	 * Build basic GSI lookup table to facilitate lookups
	 * and to prevent reprogramming of PCH_PIC pins (PCI GSIs).
	 */
	/* irq_base is 32 bit address with acpi method */
	entries = (((unsigned long)ls7a_readq(address) >> 48) & 0xff) + 1;

	irq_cfg = pch_pic_irq_routing(idx);
	irq_cfg->irq_base = irq_base;
	irq_cfg->irq_end = irq_base + entries - 1;
	/*
	 * The number of PCH_PIC IRQ registers (== #pins):
	 */
	pch_pics[idx].nr_registers = entries;

	pr_info("PCH_PIC[%d]: pch_pic_id %d, version %d, address 0x%x, IRQ %d-%d\n",
		idx, pch_pic_id(idx),
		pch_pic_ver(idx), pch_pic_addr(idx),
		irq_cfg->irq_base, irq_cfg->irq_end);

	nr_pch_pics++;
	msi_irqbase += entries;
}

void handle_virq(unsigned int irq, unsigned int cpu)
{
	generic_handle_irq(irq);
}

void static pch_pic_domains_init(void)
{
	struct fwnode_handle *irq_handle;
	struct pch_pic_irq *irq_cfg;
	int i;
	u64 address;

	for_each_pch_pic(i) {
		irq_cfg = pch_pic_irq_routing(i);
		if (!irq_cfg)
			continue;
		address = pch_pic_addr(i);
		irq_handle = irq_domain_alloc_fwnode((void *)address);
		if (!irq_handle) {
			panic("Unable to allocate domain handle  for pch_pic irqdomains.\n");
		}

		pch_pic_init(irq_handle,
				pch_pic_addr(i),
				PCH_PIC_SIZE,
				get_irq_route_model(),
				irq_cfg->irq_base);

	}
}

static void pch_msi_domain_init(int start, int count)
{
	struct fwnode_handle *irq_handle, *parent_handle;
	u64 msg_address;

	msg_address = loongson_sysconf.msi_address_lo;
	msg_address |= ((u64)loongson_sysconf.msi_address_hi << 32);
	irq_handle = irq_domain_alloc_fwnode((void *)msg_address);
	if (!irq_handle) {
		panic("Unable to allocate domain handle  for pch_msi irqdomain.\n");
	}

	if (get_irq_route_model() == PCH_IRQ_ROUTE_EXT) {
		parent_handle = eiointc_get_fwnode();
	} else {
		parent_handle = htvec_get_fwnode();
	}

	pch_msi_init(irq_handle, parent_handle,
			msg_address,
			get_irq_route_model() == PCH_IRQ_ROUTE_EXT,
			start,
			count);
}

static void pch_lpc_domain_init(void)
{
#ifdef CONFIG_LOONGSON_PCH_LPC
	struct fwnode_handle *irq_handle;
	struct device_node *np;
	struct irq_fwspec fwspec;
	int parent_irq;

	fwspec.fwnode = NULL;
	fwspec.param[0] = LS7A_LPC_CASCADE_IRQ;
	fwspec.param_count = 1;
	parent_irq = irq_create_fwspec_mapping(&fwspec);

	irq_handle = irq_domain_alloc_fwnode((void *)((u64)LS7A_LPC_INT_BASE));
	if (!irq_handle) {
		panic("Unable to allocate domain handle for pch_lpc irqdomain.\n");
	}

	np = of_find_compatible_node(NULL, NULL, "simple-bus");
	if (np) {
		if (of_property_read_bool(np, "enable-lpc-irq"))
			pch_lpc_init(LS7A_LPC_INT_BASE,
				LS7A_LPC_INT_SIZE,
				parent_irq,
				irq_handle);
	} else {
		pch_lpc_init(LS7A_LPC_INT_BASE,
			LS7A_LPC_INT_SIZE,
			parent_irq,
			irq_handle);
	}
#endif
}

static void eiointc_domain_init(void)
{
	struct fwnode_handle *irq_fwnode;

	irq_fwnode = irq_domain_alloc_fwnode((void *)((u64)LOONGSON_CSR_EXTIOI_ISR_BASE));
	if (!irq_fwnode) {
		panic("Unable to allocate domain handle for eiointc irqdomain.\n");
	}
	extioi_vec_init(irq_fwnode, LOONGSON_BRIDGE_IRQ);
}

static void liointc_domain_init(void)
{
#ifdef CONFIG_LOONGSON_LIOINTC
	struct fwnode_handle *irq_handle;
	struct resource __maybe_unused res;
	u32 parent_int_map[2] = {LIOINTC_VECS_TO_IP2, LIOINTC_VECS_TO_IP3};
	u32 parent_irq[2] = {LOONGSON_LINTC_IRQ, LOONGSON_BRIDGE_IRQ};

	irq_handle = irq_domain_alloc_fwnode((void *)liointc_base);
	if (!irq_handle) {
		panic("Unable to allocate domain handle for liointc irqdomain.\n");
	}
	res.start = liointc_base;
	res.end = liointc_base + LIOINTC_MEM_SIZE;
	liointc_init(&res,
			2,
			parent_irq,
			parent_int_map,
			irq_handle, pch_irq_route_model);
#endif
}

static void irqchip_init_default(void)
{
	int start, count;

	loongarch_cpu_irq_init();
	liointc_domain_init();
	pr_info("Support EXT interrupt.\n");
#ifdef CONFIG_LOONGSON_EXTIOI
	start = msi_irqbase;
	if (loongson_cpu_has_msi256)
		count = 256 - start;
	else
		count = 128 - start;
	eiointc_domain_init();
	pch_msi_domain_init(start, count);
#endif
	pch_pic_domains_init();
	pch_lpc_domain_init();
}

void __init setup_IRQ(void)
{
	if (!acpi_disabled) {
		if (loongson_sysconf.bpi_version > BPI_VERSION_V1) {
			irqchip_init();
		} else {
			irqchip_init_default();
		}
	} else {
		register_pch_pic(0, LS7A_PCH_REG_BASE,
				LOONGSON_PCH_IRQ_BASE);
		irqchip_init_default();
	}
}

static void ip2_irqdispatch(void)
{
	do_IRQ(LOONGSON_LINTC_IRQ);
}

static void ip3_irqdispatch(void)
{
	do_IRQ(LOONGSON_BRIDGE_IRQ);
}

#ifdef CONFIG_SMP
static void ip6_irqdispatch(void)
{
	loongson3_ipi_interrupt(NULL);
}
#endif

static void ip7_irqdispatch(void)
{
	do_IRQ(LOONGSON_TIMER_IRQ);
}

static void setup_vi_handler(void)
{
	pr_info("Setting up vectored interrupts\n");
	set_vi_handler(EXCCODE_IP0, ip2_irqdispatch);
	set_vi_handler(EXCCODE_IP1, ip3_irqdispatch);
	set_vi_handler(EXCCODE_PC, pmu_handle_irq);
#ifdef CONFIG_SMP
	set_vi_handler(EXCCODE_IPI, ip6_irqdispatch);
#endif
	set_vi_handler(EXCCODE_TIMER, ip7_irqdispatch);

	set_csr_excfg(ECFGF_IP0 | ECFGF_IP1 | ECFGF_IPI | ECFGF_PC);
}

#ifdef CONFIG_HOTPLUG_CPU
void handle_irq_affinity(void)
{
	struct irq_desc *desc;
	struct irq_chip *chip;
	unsigned int irq;
	unsigned long flags;
	struct cpumask *affinity;

	for_each_active_irq(irq) {
		desc = irq_to_desc(irq);
		if (!desc)
			continue;

		raw_spin_lock_irqsave(&desc->lock, flags);

		affinity = desc->irq_data.common->affinity;
		if (!cpumask_intersects(affinity, cpu_online_mask))
			cpumask_copy(affinity, cpu_online_mask);

		chip = irq_data_get_irq_chip(&desc->irq_data);
		if (chip && chip->irq_set_affinity)
			chip->irq_set_affinity(&desc->irq_data, desc->irq_data.common->affinity, true);
		raw_spin_unlock_irqrestore(&desc->lock, flags);
	}
}

void fixup_irqs(void)
{
	handle_irq_affinity();
	irq_cpu_offline();
	clear_csr_excfg(ECFG0_IM);
}
#endif

void __init arch_init_irq(void)
{
	/*
	 * Clear all of the interrupts while we change the able around a bit.
	 * int-handler is not on bootstrap
	 */
	clear_csr_excfg(ECFG0_IM);

	/* machine specific irq init */
	setup_IRQ();
	setup_vi_handler();
}
