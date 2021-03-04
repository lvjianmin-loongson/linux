/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 Loongson Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */

/*
 * Authors:
 *      Sui Jingfeng <suijingfeng@loongson.cn>
 */


#ifndef __LSDC_DRV_H__
#define __LSDC_DRV_H__

#include <drm/drm_device.h>
#include <drm/drm_encoder.h>

#include <linux/module.h>
#include <linux/platform_device.h>

#define to_loongson_crtc(x)             \
		container_of(x, struct loongson_crtc, base)

#define to_loongson_connector(x)        \
		container_of(x, struct loongson_connector, base)


#define LSDC_MAX_CRTC           2
#define LSDC_MAX_I2C            8

#define LSDC_MAX_FB_HEIGHT      4096
#define LSDC_MAX_FB_WIDTH       4096


enum loongson_dc_family {
	LSDC_CHIP_UNKNOWN = 0,
	LSDC_CHIP_2K1000 = 1, /* 2-Core SoC, 64-bit */
	LSDC_CHIP_7A1000 = 2, /* North bridges */
	LSDC_CHIP_7A1000_PLUS = 3, /* bandwidth improved */
	LSDC_CHIP_1A500 = 4, /* Reduced version of 2k1000, single core */
	LSDC_CHIP_7A2000 = 5, /* North bridges */
	LSDC_CHIP_LAST,
};

enum loongson_pc_board_type {
	LS_PCB_UNKNOWN = 0,
	LS2K1000_PC_EVB_V1_1 = 1,
	LS2K1000_PC_EVB_V1_2 = 2,
	LS1A500_PC_EVB_V1_0 = 3,
	LS3A4000_7A1000_EVB_BOARD_V1_4 = 4,
};

struct loongson_drm_device;
extern struct drm_driver loongson_drm_driver;

struct loongson_crtc {
	struct drm_crtc base;
	struct loongson_drm_device *ldev;
	unsigned int crtc_id;
	unsigned int cfg_reg;
	struct drm_plane *primary; /* Primary panel belongs to this crtc */
	struct drm_pending_vblank_event *event;
};


struct loongson_mode_info {
	bool mode_config_initialized;
	struct loongson_crtc *crtc;
	struct loongson_connector *connector;
};


struct loongson_encoder {
	struct drm_encoder base;
	int encoder_id;
	struct loongson_crtc *lcrtc; /* Binding crtc, not actual one */
};


struct loongson_connector {
	struct drm_connector base;
	struct loongson_i2c *i2c;
};

struct loongson_clock {
	/* khz units */
	uint32_t default_mclk;
	uint32_t default_sclk;
	uint32_t default_dispclk;
	uint32_t current_dispclk;
	unsigned int max_pixel_clock;
};

struct loongson_drm_device {
	struct drm_device *dev;
	struct drm_atomic_state *state;

	resource_size_t rmmio_base;
	resource_size_t rmmio_size;
	void __iomem *rmmio;
	enum loongson_dc_family chip;
	enum loongson_pc_board_type board;

	struct drm_display_mode mode;
	struct loongson_mode_info mode_info[2];
	struct drm_gem_cma_object *cursor;

	unsigned int num_crtc;
	struct loongson_crtc lcrtc[LSDC_MAX_CRTC];

	struct loongson_vbios *vbios;
	struct loongson_vbios_crtc *crtc_vbios[LSDC_MAX_CRTC];
	struct loongson_vbios_encoder *encoder_vbios[4];
	struct loongson_vbios_connector *connector_vbios[LSDC_MAX_CRTC];
	struct loongson_i2c *i2c_bus[LSDC_MAX_I2C];

	struct loongson_clock clock;
	bool suspended;
	bool cursor_showed;
	int cursor_crtc_id;
};


static inline struct loongson_drm_device *
to_loongson_private(struct drm_device *dev)
{
	return dev->dev_private;
}


enum loongson_edid_method {
	edid_method_null = 0,
	edid_method_i2c = 1,
	edid_method_vbios = 2,
	edid_method_encoder = 3,
	edid_method_max = 0xffffffff,
};


static inline bool clone_mode(struct loongson_drm_device *ldev)
{
	if (ldev->num_crtc < 2)
		return true;
	if (ldev->mode_info[0].connector->base.status !=
			connector_status_connected)
		return true;
	if (ldev->mode_info[1].connector->base.status !=
			connector_status_connected)
		return true;
	if (ldev->lcrtc[0].base.x || ldev->lcrtc[0].base.y)
		return false;
	if (ldev->lcrtc[1].base.x || ldev->lcrtc[1].base.y)
		return false;

	return true;
}

int lsn_load_pci_driver(struct drm_device *dev,
			const struct pci_device_id *ent);

int lsn_platform_driver_init(struct platform_device *pldev,
			     struct drm_device *ddev);
#endif
