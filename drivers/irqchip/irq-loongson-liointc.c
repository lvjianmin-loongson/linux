// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2020, Jiaxun Yang <jiaxun.yang@flygoat.com>
 *			Jianmin Lv <lvjianmin@loongson.cn>
 *  Loongson Local IO Interrupt Controller support
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <loongson.h>
#include <linux/irqchip/chained_irq.h>

#include <boot_param.h>
#include <asm/irq_cpu.h>

#define LIOINTC_CHIP_IRQ	32
#define LIOINTC_NUM_PARENT 	2

#define LIOINTC_INTC_CHIP_START	0x20
#define LIOINTC_REG_INTC_STATUS	(LIOINTC_INTC_CHIP_START + 0x20)
#define LIOINTC_REG_INTC_EN_STATUS	(LIOINTC_INTC_CHIP_START + 0x04)
#define LIOINTC_REG_INTC_ENABLE	(LIOINTC_INTC_CHIP_START + 0x08)
#define LIOINTC_REG_INTC_DISABLE	(LIOINTC_INTC_CHIP_START + 0x0c)
#define LIOINTC_REG_INTC_EDGE	(LIOINTC_INTC_CHIP_START + 0x18)

#define LIOINTC_SHIFT_INTx	4
#define LIOINTC_VECS_TO_IP2	0x00FFFFFE /* others */
#define LIOINTC_VECS_TO_IP3_V1	0xFF000000 /* HT1 0-7 */
#define LIOINTC_VECS_TO_IP3_V0	0x0F000001 /* HT1 0-3 and sys int 0 */

struct fwnode_handle *liointc_handle;
struct liointc_handler_data {
	struct liointc_priv	*priv;
	u32			parent_int_map;
};

struct liointc_priv {
	struct irq_chip_generic		*gc;
	struct liointc_handler_data	handler[LIOINTC_NUM_PARENT];
	u8				map_cache[LIOINTC_CHIP_IRQ];
};

struct fwnode_handle * liointc_get_fwnode(void)
{
	return liointc_handle;
}
static void liointc_chained_handle_irq(struct irq_desc *desc)
{
	struct liointc_handler_data *handler = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct irq_chip_generic *gc = handler->priv->gc;
	u32 pending;

	chained_irq_enter(chip, desc);

	pending = readl(gc->reg_base + LIOINTC_REG_INTC_STATUS);

	while (pending) {
		int bit = __ffs(pending);
		generic_handle_irq(irq_find_mapping(gc->domain, bit));
		pending &= ~BIT(bit);
	}

	chained_irq_exit(chip, desc);
}

static void liointc_resume(struct irq_chip_generic *gc)
{
	struct liointc_priv *priv = gc->private;
	int i;

	/* Disable all at first */
	writel(0xffffffff, gc->reg_base + LIOINTC_REG_INTC_DISABLE);
	/* Revert map cache */
	for (i = 0; i < LIOINTC_CHIP_IRQ; i++) {
		writeb(priv->map_cache[i], gc->reg_base + i);
	}

	/* Revert mask cache */
	writel(*gc->chip_types[0].mask_cache, gc->reg_base + LIOINTC_REG_INTC_ENABLE);
}

/*
 * Route */
int __init liointc_init(struct resource *res,
			int parent_irq_num,
			u32 *parent_irq,
			u32 *parent_int_map,
			struct fwnode_handle *domain_handle, int model)
{
	struct irq_chip_generic *gc;
	struct irq_domain *domain;
	struct irq_chip_type *ct;
	struct liointc_priv *priv;
	void __iomem *base;
	int i, err = 0;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	base = ioremap(res->start, resource_size(res));
	if (!base) {
		pr_err("%s: ioremap error\n", __func__);
		err = -EINVAL;
		goto out_free_priv;
	}

	for (i = 0; i < parent_irq_num; i++)
		priv->handler[i].parent_int_map = parent_int_map[i];

	domain = irq_domain_create_linear(domain_handle, LIOINTC_CHIP_IRQ,
					&irq_generic_chip_ops, priv);
	if (!domain) {
		pr_err("loongson-liointc: cannot add IRQ domain\n");
		err = -EINVAL;
		goto out_iounmap;
	}

	err = irq_alloc_domain_generic_chips(domain, LIOINTC_CHIP_IRQ, 1,
					"LIOINTC", handle_level_irq,
					0, IRQ_NOPROBE, 0);
	if (err) {
		pr_err("loongson-liointc: unable to register IRQ domain\n");
		err = -ENOMEM;
		goto out_free_domain;
	}

	/* Disable all IRQs */
	writel(0xffffffff, base + LIOINTC_REG_INTC_DISABLE);
	/* Set to level triggered */
	writel(0x0, base + LIOINTC_REG_INTC_EDGE);

	/* Generate parent INT part of map cache */
	for (i = 0; i < parent_irq_num; i++) {
		u32 pending = priv->handler[i].parent_int_map;

		while (pending) {
			int bit = __ffs(pending);

			priv->map_cache[bit] = BIT(i) << LIOINTC_SHIFT_INTx;
			pending &= ~BIT(bit);
		}
	}

	for (i = 0; i < LIOINTC_CHIP_IRQ; i++) {
		/* Generate core part of map cache */
		priv->map_cache[i] |= BIT(0);
		writeb(priv->map_cache[i], base + i);
	}

	gc = irq_get_domain_generic_chip(domain, 0);
	gc->private = priv;
	gc->reg_base = base;
	gc->domain = domain;
	gc->resume = liointc_resume;

	ct = gc->chip_types;
	ct->regs.enable = LIOINTC_REG_INTC_ENABLE;
	ct->regs.disable = LIOINTC_REG_INTC_DISABLE;
	ct->chip.irq_unmask = irq_gc_unmask_enable_reg;
	ct->chip.irq_mask = irq_gc_mask_disable_reg;
	ct->chip.irq_mask_ack = irq_gc_mask_disable_reg;

	gc->mask_cache = 0;
	priv->gc = gc;

	for (i = 0; i < parent_irq_num; i++) {
		if (parent_irq[i] <= 0)
			continue;
		if (parent_irq[i] == LOONGSON_BRIDGE_IRQ &&
				model == PCH_IRQ_ROUTE_EXT)
			continue;
		priv->handler[i].priv = priv;
		irq_set_chained_handler_and_data(parent_irq[i],
				liointc_chained_handle_irq, &priv->handler[i]);
	}

	liointc_handle = domain_handle;
	return 0;
out_free_domain:
	irq_domain_remove(domain);
out_iounmap:
	iounmap(base);
out_free_priv:
	kfree(priv);

	return err;
}
#ifdef CONFIG_ACPI
static int __init liointc_acpi_init_v1(struct acpi_subtable_header *header,
				   const unsigned long end)
{
	struct acpi_madt_liointc *liointc_entry;
	struct fwnode_handle *irq_handle;
	struct irq_fwspec fwspec;
	u32 parent_int_map[LIOINTC_NUM_PARENT];
	u32 parent_irq[LIOINTC_NUM_PARENT];
	int parent_irq_num;
	int i;
	struct resource res;
	liointc_entry = (struct acpi_madt_liointc *)header;

	irq_handle = irq_domain_alloc_named_fwnode("liointc");
	if (!irq_handle) {
		pr_err("Unable to allocate domain handle\n");
		return -ENOMEM;
	}

	parent_irq_num = 0;
	for (i = 0; i < LIOINTC_NUM_PARENT; i++) {
		if (liointc_entry->cascade[i] > 0) {
			fwspec.fwnode = coreintc_get_fwnode();
			fwspec.param[0] = liointc_entry->cascade[i];
			fwspec.param_count = 1;
			parent_irq[i] = irq_create_fwspec_mapping(&fwspec);
			parent_int_map[i] = liointc_entry->cascade_map[i];
			parent_irq_num++;
		}
	}
	if (parent_irq_num == 0) {
		return -EINVAL;
	}
	res.start = liointc_entry->address;
	res.end = liointc_entry->address + liointc_entry->size;
	liointc_init(&res, parent_irq_num,
			parent_irq,
			parent_int_map,
			irq_handle, get_irq_route_model());
	return 0;
}
IRQCHIP_ACPI_DECLARE(liointc_v1, ACPI_MADT_TYPE_LIOINTC,
		NULL, ACPI_MADT_LIOINTC_VERSION_V1,
		liointc_acpi_init_v1);
#endif
