// SPDX-License-Identifier: GPL-2.0
#include <linux/io.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/ioport.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/pm_wakeirq.h>
#include <loongson-pch.h>
#include <loongson.h>

typedef enum {
	ACPI_PCI_HOTPLUG_STATUS = 1 << 1,
	ACPI_CPU_HOTPLUG_STATUS = 1 << 2,
	ACPI_MEMORY_HOTPLUG_STATUS = 1 << 3,
	ACPI_PWRBT_STATUS = 1 << 8,
	ACPI_PCIE_WAKEUP_STATUS = 1 << 14,
	ACPI_WAKE_STATUS = 1 << 15,
} AcpiEventStatusBits;

static ATOMIC_NOTIFIER_HEAD(loongarch_hotplug_notifier_list);
static int acpi_irq;
static int acpi_hotplug_mask = 0;
static void *gpe0_status_reg;
static void *gpe0_enable_reg;
static void *acpi_status_reg;
static void *acpi_enable_reg;
static void *acpi_control_reg;
static struct input_dev *button;
static bool pcie_wake_enabled;

extern enum pch_type loongson_pch_type;

static void acpi_hw_clear_status(void)
{
	u16 value;

	/* PMStatus: Clear WakeStatus/PwrBtnStatus */
	value = readw(acpi_status_reg);
	value |= (ACPI_PWRBT_STATUS |
			ACPI_PCIE_WAKEUP_STATUS |
			ACPI_WAKE_STATUS);
	writew(value, acpi_status_reg);

	/* GPEStatus: Clear all generated events */
	writel(readl(gpe0_status_reg), gpe0_status_reg);
}

void acpi_sleep_prepare(void)
{
	acpi_hw_clear_status();
	if (pcie_wake_enabled) {
		u16 value;
		value = readw(acpi_enable_reg);
		value &= (~ACPI_PCIE_WAKEUP_STATUS);
		writew(value, acpi_enable_reg);
	}
}

void acpi_sleep_complete(void)
{
	acpi_hw_clear_status();
}

static irqreturn_t acpi_int_routine(int irq, void *dev_id)
{
	u16 value;
	int handled = IRQ_NONE;

	/* PMStatus: Check PwrBtnStatus */
	value = readw(acpi_status_reg);
	if (value & ACPI_PWRBT_STATUS) {
		writew(ACPI_PWRBT_STATUS, acpi_status_reg);
		pr_info("Power Button pressed...\n");
		input_report_key(button, KEY_POWER, 1);
		input_sync(button);
		input_report_key(button, KEY_POWER, 0);
		input_sync(button);
		handled = IRQ_HANDLED;
	}

	if (value & ACPI_PCIE_WAKEUP_STATUS) {
		value = readw(acpi_enable_reg);
		value |= ACPI_PCIE_WAKEUP_STATUS;
		writew(value, acpi_enable_reg);
		writew(ACPI_PCIE_WAKEUP_STATUS, acpi_status_reg);
		handled = IRQ_HANDLED;
	}

	value = readw(gpe0_status_reg);
	if (value & acpi_hotplug_mask) {
		writew(value, gpe0_status_reg);
		printk("ACPI GPE HOTPLUG event 0x%x \n", value);
		atomic_notifier_call_chain(&loongarch_hotplug_notifier_list, value, NULL);
		handled = IRQ_HANDLED;
	}

	return handled;
}

int __init power_button_init(void)
{
	int ret;
	if (!acpi_irq)
		return -ENODEV;

	button = input_allocate_device();
	if (!button)
		return -ENOMEM;

	button->name = "ACPI Power Button";
	button->phys = "acpi/button/input0";
	button->id.bustype = BUS_HOST;
	button->dev.parent = NULL;
	input_set_capability(button, EV_KEY, KEY_POWER);

	ret = request_irq(acpi_irq, acpi_int_routine, IRQF_SHARED, "acpi", acpi_int_routine);
	if (ret) {
		pr_err("ACPI Power Button Driver: Request irq %d failed!\n", acpi_irq);
		return -EFAULT;
	}

	ret = input_register_device(button);
	if (ret) {
		input_free_device(button);
		return ret;
	}

	dev_pm_set_wake_irq(&button->dev, acpi_irq);
	device_set_wakeup_capable(&button->dev, true);
	device_set_wakeup_enable(&button->dev, true);
	pr_info("ACPI Power Button Driver: Init successful!\n");

	return 0;
}

void acpi_registers_setup(void)
{
	u32 value;

	/* SCI_EN set */
	value = readw(acpi_control_reg);
	value |= 1;
	writew(value, acpi_control_reg);

	/* PMEnable: Enable PwrBtn */
	value = readw(acpi_enable_reg);
	value |= ACPI_PWRBT_STATUS;
	writew(value, acpi_enable_reg);

	value = readl(gpe0_enable_reg);
#ifdef CONFIG_HOTPLUG_PCI
	value |= ACPI_PCI_HOTPLUG_STATUS;
	acpi_hotplug_mask |= ACPI_PCI_HOTPLUG_STATUS;
#endif

#ifdef CONFIG_HOTPLUG_CPU
	value |= ACPI_CPU_HOTPLUG_STATUS;
	acpi_hotplug_mask |= ACPI_CPU_HOTPLUG_STATUS;
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
	value |= ACPI_MEMORY_HOTPLUG_STATUS;
	acpi_hotplug_mask |= ACPI_MEMORY_HOTPLUG_STATUS;
#endif
	if (acpi_hotplug_mask)
		writel(value, gpe0_enable_reg);
}

int register_loongarch_hotplug_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&loongarch_hotplug_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(register_loongarch_hotplug_notifier);

int unregister_loongarch_hotplug_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&loongarch_hotplug_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(unregister_loongarch_hotplug_notifier);


int __init loongson_acpi_init(void)
{
	struct irq_fwspec fwspec;
	int ret;
	switch (loongson_pch_type) {
	case LS7A:
		acpi_irq = LS7A_PCH_ACPI_IRQ;
		fwspec.fwnode = NULL;
		fwspec.param[0] = acpi_irq;
		fwspec.param_count = 1;
		ret = irq_create_fwspec_mapping(&fwspec);
		if (ret >= 0)
			acpi_irq = ret;
		acpi_control_reg = LS7A_PM1_CNT_REG;
		acpi_status_reg  = LS7A_PM1_EVT_REG;
		acpi_enable_reg  = LS7A_PM1_ENA_REG;
		gpe0_status_reg  = LS7A_GPE0_STS_REG;
		gpe0_enable_reg = (void *)LS7A_GPE0_ENA_REG;
		break;
	default:
		return 0;
	}

	pcie_wake_enabled = !(readw(acpi_enable_reg) & ACPI_PCIE_WAKEUP_STATUS);
	acpi_registers_setup();
	acpi_hw_clear_status();

	return 0;
}
