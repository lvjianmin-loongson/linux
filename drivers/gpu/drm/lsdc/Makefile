#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI)
#

ccflags-y := -Iinclude/drm
lsdc-y := \
	lsdc_drv.o \
	lsdc_irq.o \
	lsdc_crtc.o \
	lsdc_plane.o \
	lsdc_encoder.o \
	lsdc_connector.o \
	lsdc_hardware.o \
	lsdc_i2c.o \
	lsdc_cursor.o \
	lsdc_vbios.o

lsdc-$(CONFIG_DRM_LSDC_PCI_DRIVER) += lsdc_pci_drv.o
lsdc-$(CONFIG_DRM_LSDC_PLATFORM_DRIVER) += lsdc_platform_drv.o

obj-$(CONFIG_DRM_LSDC) += lsdc.o
