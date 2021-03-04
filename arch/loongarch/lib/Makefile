# SPDX-License-Identifier: GPL-2.0
#
# Makefile for LOONGARCH-specific library files..
#

lib-y	+= csum_partial.o delay.o memcpy.o memset.o \
	   strncpy_user.o strnlen_user.o

obj-y			+= iomap.o
obj-$(CONFIG_PCI)	+= iomap-pci.o
lib-$(CONFIG_GENERIC_CSUM)	:= $(filter-out csum_partial.o, $(lib-y))
obj-$(CONFIG_CPU_GENERIC_DUMP_TLB) += dump_tlb.o
