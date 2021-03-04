#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/msi.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <loongson-pch.h>
#include <loongson.h>
#include <irq.h>
#include <boot_param.h>

static bool msix_enable = 1;
core_param(msix, msix_enable, bool, 0664);

#ifdef CONFIG_LOONGSON_PCH_MSI
extern struct fwnode_handle *msi_irqdomain_handle(void);
static int setup_msidomain_irq(struct pci_dev *pdev, int nvec)
{
	struct irq_domain *msi_domain;
	msi_domain = irq_find_matching_fwnode(msi_irqdomain_handle(), DOMAIN_BUS_PCI_MSI);
	if (msi_domain == NULL)
		return -ENOSYS;

	return msi_domain_alloc_irqs(msi_domain, &pdev->dev, nvec);
}
static int teardown_msidomain_irq(unsigned int irq)
{
	struct irq_domain *msi_domain;
	msi_domain = irq_find_matching_fwnode(msi_irqdomain_handle(), DOMAIN_BUS_PCI_MSI);
	if (msi_domain)
		irq_domain_free_irqs(irq, 1);

	return 0;
}
int arch_setup_msi_irqs(struct pci_dev *dev, int nvec, int type)
{
	if (!pci_msi_enabled())
		return -ENOSPC;

	if (type == PCI_CAP_ID_MSIX && !msix_enable)
		return -ENOSPC;

	if (type == PCI_CAP_ID_MSI && nvec > 1)
		return 1;

	return setup_msidomain_irq(dev, nvec);
}

void arch_teardown_msi_irq(unsigned int irq)
{
	teardown_msidomain_irq(irq);
}
#endif
