#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI)
#

ccflags-y := -Iinclude/drm
loongson-y := loongson_drv.o \
	loongson_irq.o \
	loongson_encoder.o \
	loongson_crtc.o \
	loongson_connector.o \
	loongson_fb.o \
	loongson_cursor.o \
	loongson_vbios.o \
	loongson_ttm.o \
	loongson_debugfs.o \
	loongson_i2c.o \
	loongson_bridge.o
obj-$(CONFIG_DRM_LOONGSON_VGA)	+= loongson.o
loongson-y += bridge/it66121_drv.o
