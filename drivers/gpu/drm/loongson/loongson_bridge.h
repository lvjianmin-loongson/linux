/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2020 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __LOONGSON_BRIDGE_H__
#define __LOONGSON_BRIDGE_H__

#include "drm/drm_bridge.h"

#define ENCODER_OBJECT_ID_NONE 0x00
#define ENCODER_OBJECT_ID_INTERNAL_DVO 0x01
#define ENCODER_OBJECT_ID_INTERNAL_HDMI 0x02
#define ENCODER_OBJECT_ID_VGA_CH7055 0x10
#define ENCODER_OBJECT_ID_VGA_ADV7125 0x11
#define ENCODER_OBJECT_ID_DVI_TFP410 0x20
#define ENCODER_OBJECT_ID_HDMI_IT66121 0x30
#define ENCODER_OBJECT_ID_HDMI_SIL9022 0x31
#define ENCODER_OBJECT_ID_HDMI_LT8618 0x32
#define ENCODER_OBJECT_ID_EDP_NCS8805 0x40

#define CHIP_NAME_SIZE 20

struct bridge_resource {
	int encoder_obj;
	char chip_name[CHIP_NAME_SIZE];
	unsigned int i2c_bus_num;
	unsigned short i2c_dev_addr;
	unsigned int irq_gpio;
	unsigned int gpio_placement;

	int mode_info_index;
	struct loongson_device *ldev;
	struct loongson_encoder *ls_encoder;
};

int it66121_init(struct bridge_resource *res);

#endif
