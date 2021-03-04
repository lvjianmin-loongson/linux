// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2020, Jianmin Lv <lvjianmin@loongson.cn>
 *  Loongson Extend I/O Interrupt Vector support
 */

#define pr_fmt(fmt) "extioi: " fmt

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/syscore_ops.h>
#include <asm/irq_cpu.h>

#define VEC_REG_COUNT		4
#define VEC_COUNT_PER_REG	64
#define VEC_COUNT		(VEC_REG_COUNT * VEC_COUNT_PER_REG)
#define VEC_REG_IDX(irq_id)	((irq_id) / VEC_COUNT_PER_REG)
#define VEC_REG_BIT(irq_id)     ((irq_id) % VEC_COUNT_PER_REG)

struct extioi {
	struct irq_domain	*extioi_domain;
	struct fwnode_handle	*domain_handle;
} *extioi_priv;

unsigned int extioi_en[CSR_EXTIOI_VECTOR_NUM/32];

static struct fwnode_handle *irq_fwnode;

struct fwnode_handle * eiointc_get_fwnode(void)
{
	return irq_fwnode;
}

static void extioi_set_irq_route(int pos, const struct cpumask *affinity)
{
	uint32_t pos_off;
	unsigned int node;
	unsigned int nodemap, route_node;
	unsigned char coremap[MAX_NUMNODES];
	uint32_t data, data_byte, data_mask;
	int cpu;

	pos_off = pos & ~3;
	data_byte = pos & (3);
	data_mask = ~BIT_MASK(data_byte) & 0xf;
	nodemap = 0;
	memset(coremap, 0, sizeof(unsigned char) * MAX_NUMNODES);

	/* calculate nodemap and coremap of target irq */
	for_each_cpu(cpu, affinity) {
		nodemap |= (1 << cpu_to_node(cpu));
		coremap[cpu_to_node(cpu)] |=
			(1 << (cpu_logical_map(cpu) %
			       loongson_sysconf.cores_per_node));
	}

	for_each_online_node(node) {
		data = 0ULL;

		/* Node 0 is in charge of inter-node interrupt dispatch */
		route_node = (node == 0) ? nodemap : (1 << node);
		data |= ((coremap[node] | (route_node << 4))
				<< (data_byte * 8));

		csr_any_send(LOONGSON_CSR_EXTIOI_ROUTE_BASE + pos_off,
				data, data_mask, node);
	}

}

static DEFINE_SPINLOCK(affinity_lock);
int ext_set_irq_affinity(struct irq_data *d, const struct cpumask *affinity,
			  bool force)
{
	uint32_t vector, pos_off;
	struct cpumask new_affinity;
	unsigned long flags;

	if (!IS_ENABLED(CONFIG_SMP))
		return -EPERM;

	spin_lock_irqsave(&affinity_lock, flags);

	if (!cpumask_intersects(affinity, cpu_online_mask)) {
		spin_unlock_irqrestore(&affinity_lock, flags);
		return -EINVAL;
	}
	cpumask_and(&new_affinity, affinity, cpu_online_mask);
	cpumask_copy(d->common->affinity, &new_affinity);

	/*
	 * control interrupt enable or disalbe through cpu 0
	 * which is reponsible for dispatching interrupts.
	 */
	vector = d->hwirq;
	pos_off = vector >> 5;

	csr_any_send(LOONGSON_CSR_EXTIOI_EN_BASE + (pos_off << 2),
			extioi_en[pos_off] &
			(~((1 << (vector & 0x1F)))), 0x0, 0);
	extioi_set_irq_route(vector, &new_affinity);
	csr_any_send(LOONGSON_CSR_EXTIOI_EN_BASE + (pos_off << 2),
			extioi_en[pos_off], 0x0, 0);

	spin_unlock_irqrestore(&affinity_lock, flags);
	irq_data_update_effective_affinity(d, &new_affinity);

	return IRQ_SET_MASK_OK_NOCOPY;
}

void extioi_init(void)
{
	int i, j;
	uint32_t data;
	uint64_t tmp;

	/* init irq en bitmap */
	if (smp_processor_id() == loongson_sysconf.boot_cpu_id) {
		for (i = 0; i < CSR_EXTIOI_VECTOR_NUM/32; i++)
			extioi_en[i] = -1;
	}

	if (cpu_logical_map(smp_processor_id())
			% loongson_sysconf.cores_per_node == 0) {

		tmp = csr_readq(LOONGSON_CSR_OTHER_FUNC)
			| CSR_OTHER_FUNC_EXT_INT_EN;
		csr_writeq(tmp, LOONGSON_CSR_OTHER_FUNC);


		for (j = 0; j < 8; j++) {
			data = ((((j << 1) + 1) << 16) | (j << 1));
			csr_writel(data,
				LOONGSON_CSR_EXTIOI_NODEMAP_BASE + j*4);
		}

		for (j = 0; j < 2; j++) {
			data = 0x02020202; /* route to IP3 */
			csr_writel(data, LOONGSON_CSR_EXTIOI_IPMAP_BASE + j*4);
		}

		for (j = 0; j < CSR_EXTIOI_VECTOR_NUM/4; j++) {
			data = 0x11111111; /* route to node 0 core 0 */
			csr_writel(data, LOONGSON_CSR_EXTIOI_ROUTE_BASE + j*4);
		}

		for (j = 0; j < CSR_EXTIOI_VECTOR_NUM/32; j++) {
			data = -1;
			csr_writel(data, LOONGSON_CSR_EXTIOI_BOUNCE_BASE + j*4);
			csr_writel(data, LOONGSON_CSR_EXTIOI_EN_BASE + j*4);
		}
	}
}
static void extioi_irq_dispatch(struct irq_desc *desc)
{
	int i;
	u64 pending;
	bool handled = false;
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct extioi *priv = irq_desc_get_handler_data(desc);
	chained_irq_enter(chip, desc);

	for (i = 0; i < VEC_REG_COUNT; i++) {
		pending = csr_readq(LOONGSON_CSR_EXTIOI_ISR_BASE + (i << 3));
		csr_writeq(pending, LOONGSON_CSR_EXTIOI_ISR_BASE + (i << 3));
		while (pending) {
			int bit = __ffs(pending);
			int virq = irq_linear_revmap(priv->extioi_domain,
					bit + VEC_COUNT_PER_REG * i);
			if (virq > 0) generic_handle_irq(virq);
			pending &= ~BIT(bit);
			handled = true;
		}
	}

	if (!handled)
		spurious_interrupt();

	chained_irq_exit(chip, desc);
}

static void extioi_ack_irq(struct irq_data *d)
{
}

static void extioi_mask_irq(struct irq_data *d)
{
}

static void extioi_unmask_irq(struct irq_data *d)
{
}
static struct irq_chip extioi_irq_chip = {
	.name			= "EIOINTC",
	.irq_ack		= extioi_ack_irq,
	.irq_mask		= extioi_mask_irq,
	.irq_unmask		= extioi_unmask_irq,
	.irq_set_affinity	= ext_set_irq_affinity,
};

static int extioi_domain_translate(struct irq_domain *d,
					struct irq_fwspec *fwspec,
					unsigned long *hwirq,
					unsigned int *type)
{
	if (fwspec->param_count < 1)
		return -EINVAL;
	*hwirq = fwspec->param[0];
	*type = IRQ_TYPE_NONE;
	return 0;
}

static int extioi_domain_alloc(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs, void *arg)
{
	unsigned int type, i;
	unsigned long hwirq = 0;
	struct extioi *priv = domain->host_data;

	extioi_domain_translate(domain, arg, &hwirq, &type);
	for (i = 0; i < nr_irqs; i++) {
		irq_domain_set_info(domain, virq + i, hwirq + i, &extioi_irq_chip,
					priv, handle_edge_irq, NULL, NULL);
	}

	return 0;
}

static void extioi_domain_free(struct irq_domain *domain, unsigned int virq,
				unsigned int nr_irqs)
{
	int i;

	for (i = 0; i < nr_irqs; i++) {
		struct irq_data *d = irq_domain_get_irq_data(domain, virq + i);

		irq_set_handler(virq + i, NULL);
		irq_domain_reset_irq_data(d);
	}
}

static const struct irq_domain_ops extioi_domain_ops = {
	.translate	= extioi_domain_translate,
	.alloc		= extioi_domain_alloc,
	.free		= extioi_domain_free,
};

int extioi_vec_init(struct fwnode_handle *fwnode, int cascade)
{
	int err;

	extioi_priv = kzalloc(sizeof(*extioi_priv), GFP_KERNEL);
	if (!extioi_priv)
		return -ENOMEM;
	/* Setup IRQ domain */
	extioi_priv->extioi_domain = irq_domain_create_linear(fwnode, VEC_COUNT,
					&extioi_domain_ops, extioi_priv);
	if (!extioi_priv->extioi_domain) {
		pr_err("loongson-extioi: cannot add IRQ domain\n");
		err = -ENOMEM;
		goto failed_exit;
	}

	extioi_init();

	irq_set_chained_handler_and_data(cascade,
					extioi_irq_dispatch,
					extioi_priv);
	irq_fwnode = fwnode;
	return 0;

failed_exit:
	kfree(extioi_priv);

	return err;
}

#ifdef CONFIG_PM
static void extioi_irq_resume(void)
{
	int i;
	struct irq_desc *desc;
	extioi_init();

	for (i = 0; i < NR_IRQS; i++) {
		desc = irq_to_desc(i);
		if (desc && desc->handle_irq != NULL &&
				desc->handle_irq != handle_bad_irq) {
			ext_set_irq_affinity(&desc->irq_data, desc->irq_data.common->affinity, 0);
		}
	}
}

static int extioi_irq_suspend(void)
{
	return 0;
}

#else
#define extioi_irq_suspend NULL
#define extioi_irq_resume NULL
#endif

static struct syscore_ops extioi_irq_syscore_ops = {
	.suspend = extioi_irq_suspend,
	.resume = extioi_irq_resume,
};

static int __init extioi_init_syscore_ops(void)
{
	if (extioi_priv)
		register_syscore_ops(&extioi_irq_syscore_ops);
	return 0;
}
device_initcall(extioi_init_syscore_ops);
#ifdef CONFIG_ACPI
static int __init eiointc_acpi_init_v1(struct acpi_subtable_header *header,
				   const unsigned long end)
{
	struct acpi_madt_eiointc *eiointc_entry;
	struct irq_fwspec fwspec;
	struct fwnode_handle *fwnode;
	int parent_irq;
	eiointc_entry = (struct acpi_madt_eiointc *)header;

	fwnode = irq_domain_alloc_named_fwnode("eiointc");
	if (!fwnode) {
		pr_err("Unable to allocate domain handle\n");
		return -ENOMEM;
	}

	fwspec.fwnode = coreintc_get_fwnode();
	fwspec.param[0] = eiointc_entry->cascade;
	fwspec.param_count = 1;
	parent_irq = irq_create_fwspec_mapping(&fwspec);
	if (parent_irq > 0)
		extioi_vec_init(fwnode, parent_irq);
	return 0;
}
IRQCHIP_ACPI_DECLARE(eiointc_v1, ACPI_MADT_TYPE_EIOINTC,
		NULL, ACPI_MADT_EIOINTC_VERSION_V1,
		eiointc_acpi_init_v1);
#endif
