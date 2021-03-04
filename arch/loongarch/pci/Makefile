# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the PCI specific kernel interface routines under Linux.
#

obj-y				+= pci.o
obj-$(CONFIG_ACPI)		+= acpi.o

#
# These are still pretty much in the old state, watch, go blind.
#
obj-$(CONFIG_LOONGSON_MACH3X)	+= pci-loongson.o
obj-$(CONFIG_PCI_MMCONFIG)	+= mmconfig.o
