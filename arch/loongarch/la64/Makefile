
#
# Common code for all Loongson based systems
#

obj-y += setup.o init.o cmdline.o env.o reset.o\
	mem.o machtype.o platform.o serial.o efi.o boardinfo.o ioremap.o

#
# Serial port support
#
obj-$(CONFIG_EARLY_PRINTK) += early_printk.o
obj-$(CONFIG_LOONGSON_UART_BASE) += uart_base.o
obj-$(CONFIG_LOONGSON_MC146818) += rtc.o

obj-y += irq.o

obj-$(CONFIG_PCI_MSI)  += msi.o

#
# All Loongson-3 family machines
#

obj-y			+= platform.o dma.o clock.o ec_wpce775l.o workarounds.o \
			ec_npce985x.o ec_it8528.o

obj-$(CONFIG_SMP)	+= smp.o

obj-$(CONFIG_NUMA)	+= numa.o
