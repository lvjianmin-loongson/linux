// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include "drm/drmP.h"

#include "loongson_bridge.h"
#include "bridge/it66121_drv.h"
#include "loongson_drv.h"
#include "loongson_vbios.h"

static char *get_encoder_chip_name(int encoder_obj)
{
	switch (encoder_obj) {
	case ENCODER_OBJECT_ID_HDMI_IT66121:
		return "it66121";
	case ENCODER_OBJECT_ID_HDMI_SIL9022:
		return "sil902x";
	default:
		DRM_WARN("No matching encoder chip %d, using default\n",
			 encoder_obj);
		return "unknown";
	}
}

static int loongson_vbios_parse_bridge_resource(struct bridge_resource *res)
{
	struct loongson_device *ldev;
	int encoder_id;
	int encoder_obj;
	const char *chip_name;
	unsigned int i2c_bus_num;
	unsigned short i2c_dev_addr;
	unsigned int irq_gpio;
	unsigned int gpio_placement;

	ldev = res->ldev;
	encoder_id = res->ls_encoder->encoder_id;
	encoder_obj = get_encoder_chip(ldev, encoder_id);
	chip_name = get_encoder_chip_name(encoder_obj);
	i2c_dev_addr = get_encoder_chip_addr(ldev, encoder_id);
	i2c_bus_num = res->ls_encoder->i2c_id;
	irq_gpio = get_connector_irq_gpio(ldev, encoder_id);
	gpio_placement = get_connector_gpio_placement(ldev, encoder_id);
	if (gpio_placement < 0) {
		DRM_ERROR("Failed to parse bridge resource\n");
		return -EINVAL;
	}

	res->encoder_obj = encoder_obj;
	snprintf(res->chip_name, CHIP_NAME_SIZE, "%s", chip_name);
	res->i2c_bus_num = i2c_bus_num;
	res->i2c_dev_addr = i2c_dev_addr;
	res->irq_gpio = irq_gpio;
	res->gpio_placement = gpio_placement;

	DRM_INFO("Parse resource: Encoder [0x%x-%s]: i2c%d-%x irq(%d,%d)\n",
		 res->encoder_obj, res->chip_name, res->i2c_bus_num,
		 res->i2c_dev_addr, res->gpio_placement, res->irq_gpio);

	return 0;
}

static int legacy_connector_init(struct bridge_resource *res)
{
	int i;
	struct loongson_device *ldev;
	struct loongson_encoder *ls_encoder;
	struct loongson_connector *ls_connector;

	i = res->mode_info_index;
	ldev = res->ldev;
	ls_encoder = res->ls_encoder;
	DRM_INFO("Legacy connector-%d init\n", i);
	res->i2c_dev_addr = 0x50;
	ls_connector = loongson_connector_init(ldev, ls_encoder->connector_id);
	if (ls_connector == NULL) {
		DRM_ERROR("Failed to initialize Legacy connector[%d]\n", i);
		return -ENODEV;
	}
	drm_connector_attach_encoder(&ls_connector->base, &ls_encoder->base);
	ldev->mode_info[i].connector = ls_connector;
	ldev->mode_info[i].mode_config_initialized = true;

	return 0;
}

static int loongson_encoder_obj_select(struct bridge_resource *res)
{
	switch (res->encoder_obj) {
	case ENCODER_OBJECT_ID_NONE:
	case ENCODER_OBJECT_ID_INTERNAL_DVO:
		legacy_connector_init(res);
		break;
	case ENCODER_OBJECT_ID_HDMI_IT66121:
		loongson_it66121_init(res);
		break;

	default:
		DRM_ERROR("No matching chip can be selected!\n");
		break;
	}

	return 0;
}

int loongson_bridge_bind(struct loongson_device *ldev,
			 struct loongson_encoder *ls_encoder, int index)
{
	int ret;
	struct bridge_resource *res;

	res = kzalloc(sizeof(struct bridge_resource), GFP_KERNEL);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
		return ret;
	}

	res->ldev = ldev;
	res->ls_encoder = ls_encoder;
	res->mode_info_index = index;
	loongson_vbios_parse_bridge_resource(res);
	loongson_encoder_obj_select(res);

	return 0;
}
