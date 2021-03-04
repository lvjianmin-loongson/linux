/*
 * Copyright (c) 2018 Loongson Technology Co., Ltd.
 * Authors:
 *	Zhu Chen <zhuchen@loongson.cn>
 *	Fang Yaling <fangyaling@loongson.cn>
 *	Zhang Dandan <zhangdandan@loongson.cn>
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/pm_runtime.h>
#include <drm/drm_edid.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include "loongson_drv.h"
#include "loongson_vbios.h"
#include "loongson_i2c.h"

/**
 * loongson_connector_best_encoder
 *
 * @connector: point to the drm_connector structure
 *
 * Select the best encoder for the given connector.Used by both the helpers
 * (in the drm_atomic_helper_check_modeset() function)and in the legacy CRTC
 * helpers
 */
static struct drm_encoder *
loongson_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];
	/* pick the encoder ids */
	if (enc_id)
		return drm_encoder_find(connector->dev, NULL, enc_id);
	return NULL;
}

/**
 * loongson_do_probe_ddc_edid
 *
 * @adapter: I2C device adapter
 *
 * Try to fetch EDID information by calling I2C driver functions
 */
static bool loongson_do_probe_ddc_edid(struct i2c_adapter *adapter,
				       unsigned char *buf)
{
	unsigned char start = 0x0;
	unsigned int che_tmp = 0;
	unsigned int i;
	struct i2c_msg msgs[] = { {
					  .addr = DDC_ADDR,
					  .flags = 0,
					  .len = 1,
					  .buf = &start,
				  },
				  {
					  .addr = DDC_ADDR,
					  .flags = I2C_M_RD,
					  .len = EDID_LENGTH * 2,
					  .buf = buf,
				  } };
	if (i2c_transfer(adapter, msgs, 2) == 2) {
		if (buf[126] != 0) {
			buf[126] = 0;
			che_tmp = 0;
			for (i = 0; i < 127; i++) {
				che_tmp += buf[i];
			}
			buf[127] = 256 - (che_tmp) % 256;
		}
		if (!drm_edid_block_valid(buf, 0, true, NULL)) {
			dev_warn_once(&adapter->dev, "Invalid EDID block\n");
			return false;
		}
	} else {
		dev_warn_once(&adapter->dev, "unable to read EDID block\n");
		return false;
	}
	return true;
}

/**
 * get_edid_i2c
 *
 * According to i2c bus,acquire screen information
 */
static bool get_edid_i2c(struct loongson_connector *ls_connector, u8 *edid)
{
	struct loongson_i2c *i2c = ls_connector->i2c;
	bool ret = false;

	if (i2c != NULL && i2c->adapter != NULL)
		ret = loongson_do_probe_ddc_edid(i2c->adapter, edid);
	else
		DRM_INFO_ONCE("get loongson connector adapter err\n");

	return ret;
}

/**
 * loongson_get_modes
 *
 * @connetcor: central DRM connector control structure
 *
 * Fill in all modes currently valid for the sink into
 * the connector->probed_modes list.
 * It should also update the EDID property by calling
 * drm_mode_connector_update_edid_property().
 */
static int loongson_get_modes(struct drm_connector *connector)
{
	struct loongson_connector *ls_connector =
		to_loongson_connector(connector);
	u8 edid[EDID_LENGTH * 2];
	u32 edid_method = ls_connector->edid_method;
	u32 size = sizeof(u8) * EDID_LENGTH * 2;
	bool success = true;
	int ret = -1;

	switch (edid_method) {
	case via_vbios:
		memcpy(edid, ls_connector->vbios_edid, size);
		break;
	case via_null:
	case via_max:
	case via_encoder:
	case via_i2c:
	default:
		success = get_edid_i2c(ls_connector, edid);
	}

	if (success) {
		drm_connector_update_edid_property(connector,
						   (struct edid *)edid);
		ret = drm_add_edid_modes(connector, (struct edid *)edid);
	} else {
		DRM_DEBUG_KMS("Add default modes when noedid\n");
		ret += drm_add_modes_noedid(connector, 1920, 1080);
		ret += drm_add_modes_noedid(connector, 1024, 768);
	}

	return ret;
}

/*
 * loongson_mode_valid
 *
 * @connector: point to the drm connector
 * @mode: point to the drm_connector structure
 *
 * Validate a mode for a connector, irrespective of the specific
 * display configuration
 */
static int loongson_mode_valid(struct drm_connector *connector,
			       struct drm_display_mode *mode)
{
	if (mode->clock > 173000)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static bool is_connected(struct loongson_connector *ls_connector)
{
	unsigned char start = 0x0;
	struct i2c_adapter *adapter;
	struct i2c_msg msgs = {
		.addr = DDC_ADDR,
		.flags = 0,
		.len = 1,
		.buf = &start,
	};

	if (!ls_connector->i2c)
		return false;

	adapter = ls_connector->i2c->adapter;
	if (i2c_transfer(adapter, &msgs, 1) != 1) {
		DRM_DEBUG_KMS("display-%d not connect\n", ls_connector->id);
		return false;
	}

	return true;
}

/**
 * loongson_connector_detect
 *
 * @connector: point to drm_connector
 * @force: bool
 *
 * Check to see if anything is attached to the connector.
 * The parameter force is set to false whilst polling,
 * true when checking the connector due to a user request
 */
static enum drm_connector_status
loongson_connector_detect(struct drm_connector *connector, bool force)
{
	struct loongson_connector *ls_connector;
	enum drm_connector_status ret = connector_status_disconnected;
	enum loongson_edid_method edid_method;
	int i;

	ls_connector = to_loongson_connector(connector);
	if (ls_connector->hotplug == disable)
		return connector_status_connected;

	edid_method = ls_connector->edid_method;
	DRM_DEBUG("connect%d edid_method:%d\n", ls_connector->id, edid_method);

	switch (edid_method) {
	case via_i2c:
	case via_null:
	case via_max:
		i = pm_runtime_get_sync(connector->dev->dev);
		if (i < 0)
			break;

		if (is_connected(ls_connector))
			ret = connector_status_connected;

		pm_runtime_mark_last_busy(connector->dev->dev);
		pm_runtime_put_autosuspend(connector->dev->dev);
		break;
	case via_vbios:
	case via_encoder:
		ret = connector_status_connected;
		break;
	}
	return ret;
}

/**
 * loongson_connector_destroy
 *
 * @connector: point to the drm_connector structure
 *
 * Clean up connector resources
 */
static void loongson_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
	kfree(connector);
}

/**
 * These provide the minimum set of functions required to handle a connector
 *
 * Helper operations for connectors.These functions are used
 * by the atomic and legacy modeset helpers and by the probe helpers.
 */
static const struct drm_connector_helper_funcs loongson_connector_helper = {
	.get_modes = loongson_get_modes,
	.mode_valid = loongson_mode_valid,
	.best_encoder = loongson_connector_best_encoder,
};

/**
 * @loongson_connector_pwm_get
 *
 * @Param ls_connector loongson drm connector
 *
 * @Returns  0 is ok
 */
unsigned int loongson_connector_pwm_get(struct loongson_connector *ls_connector)
{
	u16 duty_ns, period_ns;
	u32 level;

	if (IS_ERR(ls_connector->bl.pwm))
		return 0;

	period_ns = ls_connector->bl.pwm_period;
	duty_ns = pwm_get_duty_cycle(ls_connector->bl.pwm);

	level = DIV_ROUND_UP((duty_ns * ls_connector->bl.max), period_ns);
	level = clamp(level, ls_connector->bl.min, ls_connector->bl.max);
	return level;
}

/*
 * @Function  loongson_connector_pwm_set
 *
 *@ls_connector
 **/
void loongson_connector_pwm_set(struct loongson_connector *ls_connector,
				unsigned int level)
{
	unsigned int period_ns;
	unsigned int duty_ns;

	if (IS_ERR(ls_connector->bl.pwm))
		return;

	level = clamp(level, ls_connector->bl.min, ls_connector->bl.max);
	period_ns = ls_connector->bl.pwm_period;
	duty_ns = DIV_ROUND_UP((level * period_ns), ls_connector->bl.max);

	pwm_config(ls_connector->bl.pwm, duty_ns, period_ns);
}

/**
 * @loongson_connector_bl_disable
 *
 * @Param ls_connector loongson drm connector
 * disable backlight
 */
static void
loongson_connector_bl_disable(struct loongson_connector *ls_connector)
{
	if (IS_ERR(ls_connector->bl.pwm))
		return;

	pwm_disable(ls_connector->bl.pwm);
	if (ls_connector->bl.power)
		ls_connector->bl.power(ls_connector, false);
}

/**
 * @loongson_connector_bl_enable
 *
 * @Param ls_connector loongson drm connector
 * enable backlight
 */
static void
loongson_connector_bl_enable(struct loongson_connector *ls_connector)
{
	if (IS_ERR(ls_connector->bl.pwm))
		return;

	if (ls_connector->bl.power)
		ls_connector->bl.power(ls_connector, true);
	pwm_enable(ls_connector->bl.pwm);
}

/**
 * @loongson_connector_pwm_setup
 *
 * @Param ls_connector loongson drm connector
 *
 * @Returns  0 is ok
 */
int loongson_connector_pwm_setup(struct loongson_connector *ls_connector)
{
	u16 period_ns = ls_connector->bl.pwm_period;

	pwm_set_period(ls_connector->bl.pwm, period_ns);
	pwm_set_polarity(ls_connector->bl.pwm, ls_connector->bl.pwm_polarity);

	ls_connector->bl.level = LOONGSON_BL_DEF_LEVEL;
	return 0;
}

/**
 * @loongson_connector_pwm_get_resource
 *
 * @Param ls_connector loongson drm connector
 *
 * @Returns 0 is get resource ok
 */
int loongson_connector_pwm_get_resource(struct loongson_connector *ls_connector)
{
	int ret;

	struct drm_device *dev = ls_connector->base.dev;
	struct loongson_backlight *bl = &ls_connector->bl;
	u16 pwm_id = ls_connector->bl.pwm_id;

	bl->pwm = pwm_request(pwm_id, "Loongson_bl");
	if (IS_ERR(bl->pwm)) {
		DRM_DEV_ERROR(dev->dev, "Failed to get the pwm chip\n");
		bl->pwm = NULL;
		return 1;
	}

	ret = gpio_request(LOONGSON_GPIO_LCD_VDD, "GPIO_VDD");
	if (ret) {
		DRM_DEV_INFO(ls_connector->ldev->dev->dev,
			     "EN request error!\n");
		goto free_gpu_pwm;
	}

	ret = gpio_request(LOONGSON_GPIO_LCD_EN, "GPIO_EN");
	if (ret < 0) {
		DRM_DEV_INFO(ls_connector->ldev->dev->dev,
			     "VDD request error!\n");
		goto free_gpio_vdd;
	}

	return 0;

free_gpio_vdd:
	gpio_free(LOONGSON_GPIO_LCD_VDD);
free_gpu_pwm:
	pwm_free(bl->pwm);
	bl->pwm = NULL;
	return 1;
}

/**
 * @loongson_connector_pwm_free_resource
 *
 * @Param ls_connector loongson drm connector
 */
void loongson_connector_pwm_free_resource(
	struct loongson_connector *ls_connector)
{
	struct loongson_backlight *bl = &ls_connector->bl;

	if (bl->pwm)
		pwm_free(bl->pwm);

	gpio_free(LOONGSON_GPIO_LCD_EN);
	gpio_free(LOONGSON_GPIO_LCD_VDD);
}

/**
 * @loongson_connector_lvds_power
 *
 * @ls_connector loongson  drm connector
 * @enable control power-on or power-down
 */
void loongson_connector_lvds_power(struct loongson_connector *ls_connector,
				   bool enable)
{
	if (enable) {
		struct drm_encoder *encoder;
		DRM_DEBUG_KMS("%d enabled\n", ls_connector->id);
		gpio_direction_output(LOONGSON_GPIO_LCD_VDD, 1);
		gpio_set_value(LOONGSON_GPIO_LCD_VDD, enable);

		/*Only reset current encoder*/
		encoder = ls_connector->base.encoder;
		if (encoder) {
			msleep(500);
			encoder->funcs->reset(encoder);
			msleep(10);
		}
		gpio_direction_output(LOONGSON_GPIO_LCD_EN, 1);
		gpio_set_value(LOONGSON_GPIO_LCD_EN, enable);
	} else {
		DRM_DEBUG_KMS("%d disable\n", ls_connector->id);
		gpio_set_value(LOONGSON_GPIO_LCD_EN, enable);
		msleep(300);
		gpio_set_value(LOONGSON_GPIO_LCD_VDD, enable);
	}
	ls_connector->bl.hw_enabled = enable;
}
/**
 * loongson_connector_backlight_funcs_register
 * @ls_connector loongson drm connector
 * */
void backlight_pwm_register(struct loongson_connector *ls_connector)
{
	ls_connector->bl.min = LOONGSON_BL_MIN_LEVEL;
	ls_connector->bl.max = LOONGSON_BL_MAX_LEVEL;

	ls_connector->bl.get_resource = loongson_connector_pwm_get_resource;
	ls_connector->bl.free_resource = loongson_connector_pwm_free_resource;
	ls_connector->bl.setup = loongson_connector_pwm_setup;
	ls_connector->bl.get_brightness = loongson_connector_pwm_get;
	ls_connector->bl.set_brightness = loongson_connector_pwm_set;
	ls_connector->bl.enable = loongson_connector_bl_enable;
	ls_connector->bl.disable = loongson_connector_bl_disable;
	ls_connector->bl.power = loongson_connector_lvds_power;
}

/**
 * loongson_connector_backlight_updat
 * @bd      backlight device
 * @return  operation ok retuen 0
 * */
static int loongson_connector_backlight_update(struct backlight_device *bd)
{
	bool enable;
	struct loongson_connector *ls_connector = bl_get_data(bd);
	struct loongson_backlight *backlight = &ls_connector->bl;

	enable = bd->props.power == FB_BLANK_UNBLANK;
	if (enable) {
		/*Only enable hw once*/
		if (!backlight->hw_enabled)
			ls_connector->bl.enable(ls_connector);
	} else
		ls_connector->bl.disable(ls_connector);

	backlight->level = bd->props.brightness;
	ls_connector->bl.set_brightness(ls_connector, backlight->level);

	return 0;
}
/**
 * loongson_connector_get_brightness
 * @ls_connector loongson drm connector
 * */
static int loongson_connector_get_brightness(struct backlight_device *bd)
{
	struct loongson_connector *ls_connector = bl_get_data(bd);

	if (ls_connector->bl.get_brightness)
		return ls_connector->bl.get_brightness(ls_connector);

	return -ENOEXEC;
}

static const struct backlight_ops ls_backlight_device_ops = {
	.update_status = loongson_connector_backlight_update,
	.get_brightness = loongson_connector_get_brightness,
};

/**
 * loongson_connector_backlight_register
 * @ls_connector loongson drm connector
 * @return  0 is ok .
 * */
int loongson_connector_backlight_register(
	struct loongson_connector *ls_connector)
{
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = ls_connector->bl.max;
	props.brightness = ls_connector->bl.level;

	ls_connector->bl.device =
		backlight_device_register("loongson-gpu",
					  ls_connector->base.kdev, ls_connector,
					  &ls_backlight_device_ops, &props);

	if (IS_ERR(ls_connector->bl.device)) {
		DRM_DEV_ERROR(ls_connector->ldev->dev->dev,
			      "Failed to register backlight\n");

		ls_connector->bl.device = NULL;
		return -ENODEV;
	}

	DRM_INFO("Loongson connector%s bl sysfs interface registered\n",
		 ls_connector->base.name);

	return 0;
}

/**
 * loongson_connector_pwm_init
 * @ls_connector loongson drm connector
 * */
int loongson_connector_pwm_init(struct loongson_connector *ls_connector)
{
	int ret = 1;
	struct loongson_backlight *bl = &ls_connector->bl;

	if (bl->get_resource)
		ret = bl->get_resource(ls_connector);
	if (ret) {
		DRM_DEV_INFO(ls_connector->ldev->dev->dev, "get resrouce err");
		return ret;
	}

	if (bl->setup)
		ret = bl->setup(ls_connector);
	if (ret) {
		DRM_DEV_INFO(ls_connector->ldev->dev->dev, "pwm set err");
		return ret;
	}
	return ret;
}

/**
 * loongson_connector_late_register
 * @connector  drm connector
 * @returns
 * */
int loongson_connector_late_register(struct drm_connector *connector)
{
	struct loongson_connector *ls_connector =
		to_loongson_connector(connector);
	u32 type = ls_connector->type;
	int ret;

	if (type == DRM_MODE_CONNECTOR_LVDS || type == DRM_MODE_CONNECTOR_eDP) {
		backlight_pwm_register(ls_connector);
		ret = loongson_connector_pwm_init(ls_connector);
		if (ret == 0) {
			ret = loongson_connector_backlight_register(
				ls_connector);

			if (ret == 0)
				ls_connector->bl.present = true;
		}
	}

	return 0;
}

/**
 * @loongson_connector_early_unregister
 *
 * @Param connector loongson drm connector
 */
void loongson_connector_early_unregister(struct drm_connector *connector)
{
	struct loongson_connector *ls_connector;
	struct loongson_backlight *bl;
	ls_connector = to_loongson_connector(connector);

	if (IS_ERR(ls_connector))
		return;

	bl = &ls_connector->bl;
	if (bl->present == true) {
		if (bl->free_resource)
			bl->free_resource(ls_connector);
	}
	bl->present = false;
}

/**
 * These provide the minimum set of functions required to handle a connector
 *
 * Control connectors on a given device.
 * The functions below allow the core DRM code to control connectors,
 * enumerate available modes and so on.
 */
static const struct drm_connector_funcs loongson_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = loongson_connector_detect,
	.late_register = loongson_connector_late_register,
	.early_unregister = loongson_connector_early_unregister,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = loongson_connector_destroy,
};

/**
 * loongson_connector_init
 *
 * @ldev: loongson drm device
 * @index obj connector_id:
 *
 * Vga is the interface between host and monitor
 * This function is to init vga
 */
struct loongson_connector *loongson_connector_init(struct loongson_device *ldev,
						   int index)
{
	struct drm_connector *connector;
	struct loongson_connector *ls_connector;

	ls_connector = kzalloc(sizeof(struct loongson_connector), GFP_KERNEL);
	if (!ls_connector) {
		DRM_INFO("loongson connector kzalloc failed\n");
		return NULL;
	}

	ls_connector->ldev = ldev;
	ls_connector->id = index;
	ls_connector->type = get_connector_type(ldev, index);
	ls_connector->i2c_id = get_connector_i2cid(ldev, index);
	ls_connector->hotplug = get_hotplug_mode(ldev, index);
	ls_connector->edid_method = get_edid_method(ldev, index);
	if (ls_connector->edid_method == via_vbios)
		ls_connector->vbios_edid = get_vbios_edid(ldev, index);
	ls_connector->bl.pwm_id = get_vbios_pwm(ldev, index, VBIOS_PWM_ID);
	ls_connector->bl.pwm_polarity =
		get_vbios_pwm(ldev, index, VBIOS_PWM_POLARITY);
	ls_connector->bl.pwm_period =
		get_vbios_pwm(ldev, index, VBIOS_PWM_PERIOD);

	ls_connector->i2c = &ldev->i2c_bus[index];
	if (!ls_connector->i2c)
		DRM_INFO("connector-%d match i2c-%d err\n", index,
			 ls_connector->i2c_id);

	connector = &ls_connector->base;
	connector->connector_type_id = ls_connector->type;
	drm_connector_init(ldev->dev, connector, &loongson_connector_funcs,
			   ls_connector->type);
	drm_connector_helper_add(connector, &loongson_connector_helper);
	drm_connector_register(connector);

	switch (ls_connector->hotplug) {
	case irq:
		connector->polled = DRM_CONNECTOR_POLL_HPD;
		break;
	case polling:
		connector->polled = DRM_CONNECTOR_POLL_CONNECT |
				    DRM_CONNECTOR_POLL_DISCONNECT;
		break;
	case disable:
	default:
		connector->polled = 0;
		break;
	}

	return ls_connector;
}

void loongson_connector_power_mode(struct loongson_connector *ls_connector,
				   int mode)
{
	if (!ls_connector->bl.present)
		return;

	if (mode == DRM_MODE_DPMS_ON)
		ls_connector->bl.enable(ls_connector);
	else
		ls_connector->bl.disable(ls_connector);
}
/**
 * loongson_connector_bl_resume
 *
 * @ls_connector loongson drm connector
 * */
void loongson_connector_bl_resume(struct loongson_connector *ls_connector)
{
	struct loongson_backlight *backlight = &ls_connector->bl;

	if (backlight->present == true) {
		if (backlight->hw_enabled)
			backlight->enable(ls_connector);
	}
}

/**
 * loongson_connector_resume
 *
 * @ldev loongson drm device
 * */
void loongson_connector_resume(struct loongson_device *ldev)
{
	struct loongson_mode_info *ls_mode_info;
	struct loongson_connector *ls_connector;
	int i;

	for (i = 0; i < LS_MAX_MODE_INFO; i++) {
		ls_mode_info = &ldev->mode_info[i];
		if (ls_mode_info->mode_config_initialized == true) {
			ls_connector = ls_mode_info->connector;
			loongson_connector_bl_resume(ls_connector);
		}
	}
}
