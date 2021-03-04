// SPDX-License-Identifier: GPL-2.0
/*
 * pci-loongson.c
 *
 * Copyright (C) 2012 Lemote, Inc.
 * Author: Xiang Yu, xiangy@lemote.com
 *         Chen Huacai, chenhc@lemote.com
 * Copyright (C) 2020 Loongson Technology Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/vgaarb.h>
#include <irq.h>
#include <boot_param.h>
#include <workarounds.h>
#include <loongson.h>
#include <loongson-pch.h>
#include <pci.h>

extern int pcibios_dev_init(struct pci_dev *dev);
void pci_no_msi(void);

u32 node_id_offset;
unsigned long ls7a_rwflags;
DEFINE_RWLOCK(ls7a_rwlock);
EXPORT_SYMBOL(ls7a_rwflags);
EXPORT_SYMBOL(ls7a_rwlock);

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#define PCI_byte_BAD 0
#define PCI_word_BAD (pos & 1)
#define PCI_dword_BAD (pos & 3)
#define PCI_FIND_CAP_TTL        48

#define PCI_OP_READ(size,type,len) \
static int pci_bus_read_config_##size##_nolock \
	(struct pci_bus *bus, unsigned int devfn, int pos, type *value) \
{                                                                       \
	int res;                                                        \
	u32 data = 0;                                                   \
	if (PCI_##size##_BAD) return PCIBIOS_BAD_REGISTER_NUMBER;       \
	res = bus->ops->read(bus, devfn, pos, len, &data);              \
	*value = (type)data;                                            \
	return res;                                                     \
}

PCI_OP_READ(byte, u8, 1)
PCI_OP_READ(word, u16, 2)
PCI_OP_READ(dword, u32, 4)

#ifndef loongson_ls7a_decode_year
#define loongson_ls7a_decode_year(year) ((year) + 1900)
#endif

#define RTC_TOYREAD0    0x2C
#define RTC_YEAR        0x30

int ls7a_guest_pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	int irq;
	u8 tmp;

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &tmp);
	slot = pci_common_swizzle(dev, &tmp);

	irq = LS7A_PCH_IRQ_BASE + 16 + ((slot * 4 + tmp - 1) & 0xf);
	return irq;
}
static int __pci_find_next_cap_ttl_nolock(struct pci_bus *bus, unsigned int devfn,
					  u8 pos, int cap, int *ttl)
{
	u8 id;
	u16 ent;

	pci_bus_read_config_byte_nolock(bus, devfn, pos, &pos);

	while ((*ttl)--) {
		if (pos < 0x40)
			break;
		pos &= ~3;
		pci_bus_read_config_word_nolock(bus, devfn, pos, &ent);

		id = ent & 0xff;
		if (id == 0xff)
			break;
		if (id == cap)
			return pos;
		pos = (ent >> 8);
	}
	return 0;
}

static int __pci_find_next_cap_nolock(struct pci_bus *bus, unsigned int devfn,
					u8 pos, int cap)
{
	int ttl = PCI_FIND_CAP_TTL;

	return __pci_find_next_cap_ttl_nolock(bus, devfn, pos, cap, &ttl);
}

static int __pci_bus_find_cap_start_nolock(struct pci_bus *bus,
					   unsigned int devfn, u8 hdr_type)
{
	u16 status;

	pci_bus_read_config_word_nolock(bus, devfn, PCI_STATUS, &status);
	if (!(status & PCI_STATUS_CAP_LIST))
		return 0;

	switch (hdr_type) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
		return PCI_CAPABILITY_LIST;
	case PCI_HEADER_TYPE_CARDBUS:
		return PCI_CB_CAPABILITY_LIST;
	default:
		return 0;
	}

	return 0;
}

static int pci_bus_find_capability_nolock(struct pci_bus *bus, unsigned int devfn, int cap)
{
	int pos;
	u8 hdr_type;

	pci_bus_read_config_byte_nolock(bus, devfn, PCI_HEADER_TYPE, &hdr_type);

	pos = __pci_bus_find_cap_start_nolock(bus, devfn, hdr_type & 0x7f);
	if (pos)
		pos = __pci_find_next_cap_nolock(bus, devfn, pos, cap);

	return pos;
}

static int ls7a_pci_config_access(unsigned char access_type,
		struct pci_bus *bus, unsigned int devfn,
		int where, u32 *data)
{
	u_int64_t addr;
	void *addrp;
	unsigned char busnum = bus->number;
	unsigned long long domain = pci_domain_nr(bus);
	unsigned int val;
	int device = PCI_SLOT(devfn);
	int function = PCI_FUNC(devfn);
	int reg = where & ~3;
	int pos;

	if (busnum == 0) {
		/** Filter out non-supported devices.
		 *  device 2: misc, device 21: confbus
		 */
		if (device > 23)
			return PCIBIOS_DEVICE_NOT_FOUND;
		if (reg < 0x100) {
			addr = (domain << 44) | (device << 11) | (function << 8) | reg;
			addrp = (void *)TO_UNCAC(HT1LO_PCICFG_BASE | addr);
		} else {
			addr = (domain << 44) | (device << 11) | (function << 8) | (reg & 0xff) | ((reg & 0xf00) << 16);
			addrp = (void *)(TO_UNCAC(HT1LO_EXT_PCICFG_BASE) | addr);
		}
	} else {
		if (reg < 0x100) {
			addr = (domain << 44) | (busnum << 16) | (device << 11) | (function << 8) | reg;
			addrp = (void *)TO_UNCAC(HT1LO_PCICFG_BASE_TP1 | addr);
		} else {
			addr = (domain << 44) | (busnum << 16) | (device << 11) | (function << 8) | (reg & 0xff) | ((reg & 0xf00) << 16);
			addrp = (void *)(TO_UNCAC(HT1LO_EXT_PCICFG_BASE_TP1) | addr);
		}
	}

	if (access_type == PCI_ACCESS_WRITE) {
		pos = pci_bus_find_capability_nolock(bus, devfn, PCI_CAP_ID_EXP);

		if (pos != 0 && (pos + PCI_EXP_DEVCTL) == reg)
		{
			/* need pmon/uefi configure mrrs to pcie slot max mrrs capability */
			val = *(volatile unsigned int *)addrp;
			if ((*data & PCI_EXP_DEVCTL_READRQ) > (val & PCI_EXP_DEVCTL_READRQ)) {
				printk(KERN_ERR "MAX READ REQUEST SIZE shuould not greater than the slot max capability");
				*data = *data & ~PCI_EXP_DEVCTL_READRQ;
				*data = *data | (val & PCI_EXP_DEVCTL_READRQ);
			}
		}

		*(volatile unsigned int *)addrp = cpu_to_le32(*data);
	} else {
		*data = le32_to_cpu(*(volatile unsigned int *)addrp);
		if (busnum == 0 && reg == PCI_CLASS_REVISION && *data == 0x06000001)
			*data = (PCI_CLASS_BRIDGE_PCI << 16) | (*data & 0xffff);

		if (*data == 0xffffffff) {
			*data = -1;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
	}
	return PCIBIOS_SUCCESSFUL;
}

static int ls7a_pci_pcibios_read(struct pci_bus *bus, unsigned int devfn,
				 int where, int size, u32 * val)
{
	u32 data = 0;
	int ret = ls7a_pci_config_access(PCI_ACCESS_READ,
			bus, devfn, where, &data);

	if (ret != PCIBIOS_SUCCESSFUL)
		return ret;

	if (size == 1)
		*val = (data >> ((where & 3) << 3)) & 0xff;
	else if (size == 2)
		*val = (data >> ((where & 3) << 3)) & 0xffff;
	else
		*val = data;

	return PCIBIOS_SUCCESSFUL;
}

static int ls7a_pci_pcibios_write(struct pci_bus *bus, unsigned int devfn,
				  int where, int size, u32 val)
{
	u32 data = 0;
	int ret;

	if (size == 4)
		data = val;
	else {
		ret = ls7a_pci_config_access(PCI_ACCESS_READ,
				bus, devfn, where, &data);
		if (ret != PCIBIOS_SUCCESSFUL)
			return ret;

		if (size == 1)
			data = (data & ~(0xff << ((where & 3) << 3))) |
			    (val << ((where & 3) << 3));
		else if (size == 2)
			data = (data & ~(0xffff << ((where & 3) << 3))) |
			    (val << ((where & 3) << 3));
	}

	ret = ls7a_pci_config_access(PCI_ACCESS_WRITE,
			bus, devfn, where, &data);
	if (ret != PCIBIOS_SUCCESSFUL)
		return ret;

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops ls7a_pci_ops = {
	.read = ls7a_pci_pcibios_read,
	.write = ls7a_pci_pcibios_write
};

static void pci_fixup_radeon(struct pci_dev *pdev)
{
	struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];

	if (res->start)
		return;

	if (!loongson_sysconf.vgabios_addr)
		return;

	pci_disable_rom(pdev);
	if (res->parent)
		release_resource(res);

	res->start = virt_to_phys((void *) loongson_sysconf.vgabios_addr);
	res->end   = res->start + 256*1024 - 1;
	res->flags = IORESOURCE_MEM | IORESOURCE_ROM_SHADOW |
		     IORESOURCE_PCI_FIXED;

	dev_info(&pdev->dev, "BAR %d: assigned %pR for Radeon ROM\n",
		 PCI_ROM_RESOURCE, res);
}

DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_ATI, 0x9615,
				PCI_CLASS_DISPLAY_VGA, 8, pci_fixup_radeon);


static void pci_fixup_ls7a1000(struct pci_dev *pdev)
{
	struct pci_dev *devp = NULL;

	while ((devp = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, devp))) {
		if (devp->vendor != PCI_VENDOR_ID_LOONGSON) {
			vga_set_default_device(devp);
			dev_info(&pdev->dev,
				"Overriding boot device as %X:%X\n",
				devp->vendor, devp->device);
		}
	}
}
DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_LOONGSON, PCI_DEVICE_ID_LOONGSON_DC,
		PCI_CLASS_DISPLAY_VGA, 8, pci_fixup_ls7a1000);

DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_LOONGSON, PCI_DEVICE_ID_LOONGSON_DC,
		PCI_CLASS_DISPLAY_OTHER, 8, pci_fixup_ls7a1000);


/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	dev->dev.archdata.dma_attrs = 0;
	if (loongson_sysconf.workarounds & WORKAROUND_PCIE_DMA)
		dev->dev.archdata.dma_attrs = DMA_ATTR_FORCE_SWIOTLB;

	return pcibios_dev_init(dev);
}
