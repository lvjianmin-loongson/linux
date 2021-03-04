/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2020 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IT66121_REGS_H__
#define __IT66121_REGS_H__

#include <linux/types.h>
#include <linux/regmap.h>

#include "it66121_drv.h"

/**
 * @brief Config it66121 registers attribute
 */
static const u8 it66121_reg_defaults_raw[] = {
	0x54, 0x49, 0x12, 0x16, 0x1c, 0x60, 0x00, 0x00, /* 00 */
	0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x0c, 0x08, /* 0F */
	0x96, 0xa0, 0x60, 0x20, 0x01, 0xf3, 0x82, 0xf4,
	0x00, 0x74, 0xe0, 0x03, 0x00, 0x00, 0xff, 0x00, /* 1F */
	0x08, 0xff, 0x00, 0xf7, 0x49, 0x5f, 0x4b, 0x40,
	0xec, 0xc3, 0x15, 0xe8, 0xca, 0xe5, 0x06, 0x34, /* 2F */
	0x6a, 0x67, 0x6f, 0x95, 0xee, 0x9a, 0xe5, 0x13,
	0xd5, 0xf1, 0xea, 0xcb, 0x9e, 0xce, 0x14, 0x56, /* 3F */
	0x9d, 0x88, 0xe8, 0x00, 0x68, 0xe7, 0x00, 0x00,
	0x4a, 0x22, 0xc8, 0x10, 0x2a, 0xa5, 0x00, 0xbb, /* 4F */
	0x80, 0x54, 0xcf, 0x5e, 0x0e, 0x2f, 0x8f, 0xbf,
	0x11, 0x03, 0x03, 0xfe, 0x00, 0x94, 0x37, 0x9e, /* 5F */
	0xff, 0x10, 0x88, 0x18, 0x94, 0x00, 0x00, 0x00,
	0x00, 0x30, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, /* 6F */
	0x00, 0x08, 0x00, 0x10, 0x80, 0x00, 0x6c, 0x7c,
	0x89, 0xe9, 0x2d, 0x18, 0x4a, 0xc6, 0x6a, 0xd2, /* 7F */
	0x43, 0x08, 0x03, 0x16, 0x90, 0x08, 0x3f, 0xf6,
	0x2b, 0xb3, 0xff, 0xff, 0xff, 0xc8, 0x00, 0x00, /* 8F */
	0x00, 0x6a, 0xf0, 0x35, 0xb0, 0xdb, 0xcd, 0x10,
	0x16, 0xc7, 0x46, 0xd0, 0xa7, 0xe1, 0x37, 0x44, /* 9F */
	0x77, 0xc3, 0xd6, 0xfe, 0xb2, 0x06, 0x90, 0x86,
	0x00, 0x04, 0x39, 0x0a, 0xa1, 0x6f, 0xfb, 0xc9, /* AF */
	0x18, 0x67, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, /* BF */
	0x00, 0x81, 0xb9, 0x08, 0xc0, 0x04, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, /* CF */
	0x00, 0x04, 0x00, 0xff, 0xff, 0xff, 0xff, 0x04,
	0xb6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* DF */
	0xc0, 0x41, 0xe4, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff, 0xff, 0xff, 0x7f, 0xff, 0x00, 0xff, /* EF */
	0x00, 0xff, 0xff, 0x18, 0x00, 0x02, 0xff, 0xff,
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* FF */
	0x47, 0x8c, 0x72, 0x80, 0x18, 0x00, 0x00, 0x00,
	0x22, 0x72, 0x0c, 0x5f, 0xa6, 0x3d, 0x1a, 0x8c, /* 13F */
	0x09, 0x98, 0x42, 0x98, 0x43, 0x50, 0xdf, 0x20,
	0x40, 0xa6, 0xd2, 0x99, 0xff, 0x98, 0x4c, 0x66, /* 14F */
	0x1a, 0x61, 0x49, 0xa2, 0x0f, 0xec, 0xc9, 0xff,
	0x00, 0x08, 0x19, 0x99, 0x00, 0x55, 0x00, 0x00, /* 15F */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x82,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0xf1, 0x3d, /* 16F */
	0xe6, 0xd9, 0x2d, 0xba, 0x0a, 0x07, 0x54, 0x9a,
	0xb9, 0xb9, 0x92, 0x98, 0x73, 0x5c, 0xdd, 0xf6, /* 17F */
	0x0d, 0xff, 0x4f, 0xc3, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x50, 0x02, 0x3b, 0x06, 0x83, 0x7c, /* 18F */
	0xff, 0x00, 0x27, 0xfd, 0x0a, 0xff, 0xff, 0xff,
	0x00, 0xb4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 19F */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 1AF */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xd8, 0x67, 0x67,
	0x28, 0x9a, 0xaf, 0x47, 0x2e, 0x27, 0xbf, 0x9f, /* 1BF */
};

static bool it66121_register_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case IT66121_REG_VENDOR_ID_BASE ... IT66121_REG_CFG:
	case IT66121_REG_INT_MASK_BASE ... IT66121_REG_INT_CLEAR_END:
	case IT66121_REG_SYS_CLK:
	case IT66121_REG_DDC_CTRL:
	case IT66121_REG_DDC_ADDR:
	case IT66121_REG_DDC_OFFSET:
	case IT66121_REG_DDC_FIFO_SIZE:
	case IT66121_REG_DDC_EDID_BLOCK:
	case IT66121_REG_INPUT_DATA:
	case IT66121_REG_HDMI_MODE:
	case IT66121_REG_AVI_BASE ... IT66121_REG_AVI_BASE + AVI_LENGTH:
		return false;
	case IT66121_REG_INT_STATUS_BASE ... IT66121_REG_INT_STATUS_END:
	case IT66121_REG_SYS_STATUS:
	case IT66121_REG_DDC_CMD:
	case IT66121_REG_DDC_STATUS:
	case IT66121_REG_FIFO_CONTENT:
	case IT66121_REG_AVMUTE:
	default:
		return true;
	}
}

static const struct regmap_range it66121_rw_regs_range[] = {
	regmap_reg_range(0x00, 0xff),
	regmap_reg_range(0x100, 0x1ff),
};

static const struct regmap_range it66121_ro_regs_range[] = {
	regmap_reg_range(IT66121_REG_VENDOR_ID_BASE, IT66121_REG_CHIP_REVISION),
};

static const struct regmap_range it66121_vo_regs_range[] = {
	regmap_reg_range(0x100, 0x1ff),
};

/**
 * @brief it66121 sequence
 */
static const struct reg_sequence it66121_reg_init_seq[] = {
	/* IT66121 initial batch */
	/* Initial clock */
	{ 0x0F, 0x38 },
	{ 0x62, 0x80 },
	{ 0x64, 0x90 },
	/* Initial register */
	{ 0x04, 0x20 },
	{ 0x04, 0x1D },
	{ 0x61, 0x30 },
	/* AFE for low speed */
	{ 0x62, 0x18 },
	{ 0x64, 0x1D },
	{ 0x68, 0x10 },
	/* AFE for high speed */
	{ 0x62, 0x88 },
	{ 0x64, 0x94 },
	{ 0x68, 0x00 },
	{ 0x61, 0x10 },
	{ 0x04, 0x05 },
	{ 0x05, IT66121_INT_CFG },
	{ 0x65, 0x00 },
	{ 0xD1, 0x08 },
	{ 0x65, 0x00 },
	{ 0xFF, 0xC3 },
	{ 0xFF, 0xA5 },
	{ 0x20, 0x88 },
	{ 0x37, 0x02 },
	{ 0x20, 0x08 },
	{ 0xFF, 0xFF },
	{ 0x09, 0xFF },
	{ 0x0A, 0xFF },
	{ 0x0B, 0xFF },
	{ 0x0C, 0xFF },
	{ 0x0D, 0xFF },
	{ 0x0E, 0xFF },
};

static const struct reg_sequence it66121_power_on_seq[] = {
	{ 0x0f, 0x38 },
	{ 0x05, IT66121_INT_CFG },
	/* High AFE */
	{ 0x61, 0x10 },
	{ 0x62, 0x80 },
	{ 0x64, 0x90 },
	{ 0x61, 0x00 },
	{ 0x62, 0x88 },
	{ 0x64, 0x94 },
	{ 0x68, 0x00 },
	/* Low AFE */
	{ 0x61, 0x10 },
	{ 0x62, 0x10 },
	{ 0x64, 0x19 },
	{ 0x61, 0x00 },
	{ 0x62, 0x18 },
	{ 0x64, 0x1D },
	{ 0x68, 0x00 },
};

static const struct reg_sequence it66121_power_off_seq[] = {
	/* Power off */
	{ 0x0F, 0x00 },
	{ 0x61, 0x10 },
	{ 0x62, 0x80 },
	{ 0x64, 0x80 },
	{ 0x61, 0x30 },
	{ 0x62, 0xC4 },
	{ 0x64, 0xC0 },
	{ 0x0F, 0x38 },
	{ 0x70, 0x00 },
	/* Set pin status */
	{ 0x05, IT66121_INT_CFG },
	{ 0xE0, 0x00 },
	{ 0x72, 0x00 },

};

static const struct reg_sequence it66121_afe_high_seq[] = {
	/* IT66121 initial batch */
	{ 0x0F, 0x00 },
	{ 0x61, 0x10 },
	{ 0x62, 0x88 },
	{ 0x64, 0x94 },
	/* Initial clock */
	{ 0x68, 0x00 },
	{ 0x04, 0x1D },
	{ 0x04, 0x15 },
	{ 0x61, 0x00 },
};

static const struct reg_sequence it66121_afe_low_seq[] = {
	/* IT66121 initial batch */
	{ 0x0F, 0x00 },
	{ 0x61, 0x10 },
	{ 0x62, 0x18 },
	{ 0x64, 0x1D },
	/* Initial clock */
	{ 0x68, 0x10 },
	{ 0x04, 0x1D },
	{ 0x04, 0x15 },
	{ 0x61, 0x00 },
};

static const struct reg_sequence it66121_hdmi_seq[] = {
	{ 0x0F, 0x00 },
	{ 0xC0, 0x01 },
	{ 0xC1, 0x00 },
	/* Set GCP package */
	{ 0xC6, 0x03 },
	{ 0xCD, 0x03 },
	{ 0xCE, 0x03 },
};

static const struct reg_mask_seq it66121_hdcp_seq[] = {
	{ 0x20, 0x00, 0x98 },
	{ 0x04, 0x01, 0x98 },
	{ 0x04, 0x00, 0x98 },
	{ 0x10, 0x00, 0x98 },
	/* Enable HDCP-1.2 sync detect */
	{ 0x20, 0x01, 0x98 },
	{ 0x21, 0x01, 0x98 },
};

static const struct reg_mask_seq it66121_avi_info_frame_seq[] = {
	/* 1080p */
	{ 0x0F, 0x01, 0x01 },
	{ 0x58, 0xFF, 0x10 },
	{ 0x59, 0xFF, 0x08 },
	{ 0x5A, 0xFF, 0x00 },
	{ 0x5B, 0xFF, 0x10 },
	{ 0x5C, 0xFF, 0x00 },
	/* Set AVI package check sum */
	{ 0x5D, 0xFF, 0x47 },
	{ 0x5E, 0xFF, 0x00 },
	{ 0x5F, 0xFF, 0x00 },
	{ 0x60, 0xFF, 0x00 },
	{ 0x61, 0xFF, 0x00 },
	{ 0x62, 0xFF, 0x00 },
	{ 0x63, 0xFF, 0x00 },
	{ 0x64, 0xFF, 0x00 },
	{ 0x65, 0xFF, 0x00 },
	{ 0x0F, 0x01, 0x00 },
	{ 0xCD, 0x03, 0x03 },
};

static const struct reg_sequence it66121_int_enable[] = {
	{ 0x09, 0x00 },
	{ 0x0A, 0x00 },
	{ 0x0B, 0x00 },
};

static const struct reg_sequence it66121_int_disable[] = {
	{ 0x09, 0xFF },
	{ 0x0A, 0xFF },
	{ 0x0B, 0xFF },
};

static const struct drm_display_mode cea_modes[] = {
	/* 0 - dummy, VICs start at 1 */
	{},
	/* 1 - 640x480@60Hz 4:3 */
	{
		DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656, 752,
			 800, 0, 480, 490, 492, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 2 - 720x480@60Hz 4:3 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 3 - 720x480@60Hz 16:9 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 4 - 1280x720@60Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
			 1430, 1650, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 5 - 1920x1080i@60Hz 16:9 */
	{
		DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 6 - 720(1440)x480i@60Hz 4:3 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 7 - 720(1440)x480i@60Hz 16:9 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 8 - 720(1440)x240@60Hz 4:3 */
	{
		DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739, 801,
			 858, 0, 240, 244, 247, 262, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 9 - 720(1440)x240@60Hz 16:9 */
	{
		DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739, 801,
			 858, 0, 240, 244, 247, 262, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 10 - 2880x480i@60Hz 4:3 */
	{
		DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
			 3204, 3432, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 11 - 2880x480i@60Hz 16:9 */
	{
		DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
			 3204, 3432, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 12 - 2880x240@60Hz 4:3 */
	{
		DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
			 3204, 3432, 0, 240, 244, 247, 262, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 13 - 2880x240@60Hz 16:9 */
	{
		DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
			 3204, 3432, 0, 240, 244, 247, 262, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 14 - 1440x480@60Hz 4:3 */
	{
		DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
			 1596, 1716, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 15 - 1440x480@60Hz 16:9 */
	{
		DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
			 1596, 1716, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 16 - 1920x1080@60Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 17 - 720x576@50Hz 4:3 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 18 - 720x576@50Hz 16:9 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 19 - 1280x720@50Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
			 1760, 1980, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 20 - 1920x1080i@50Hz 16:9 */
	{
		DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 21 - 720(1440)x576i@50Hz 4:3 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 22 - 720(1440)x576i@50Hz 16:9 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 23 - 720(1440)x288@50Hz 4:3 */
	{
		DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732, 795,
			 864, 0, 288, 290, 293, 312, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 24 - 720(1440)x288@50Hz 16:9 */
	{
		DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732, 795,
			 864, 0, 288, 290, 293, 312, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 25 - 2880x576i@50Hz 4:3 */
	{
		DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
			 3180, 3456, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 26 - 2880x576i@50Hz 16:9 */
	{
		DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
			 3180, 3456, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 27 - 2880x288@50Hz 4:3 */
	{
		DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
			 3180, 3456, 0, 288, 290, 293, 312, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 28 - 2880x288@50Hz 16:9 */
	{
		DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
			 3180, 3456, 0, 288, 290, 293, 312, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 29 - 1440x576@50Hz 4:3 */
	{
		DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
			 1592, 1728, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 30 - 1440x576@50Hz 16:9 */
	{
		DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
			 1592, 1728, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 31 - 1920x1080@50Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 32 - 1920x1080@24Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
			 2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 33 - 1920x1080@25Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 34 - 1920x1080@30Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 35 - 2880x480@60Hz 4:3 */
	{
		DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
			 3192, 3432, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 36 - 2880x480@60Hz 16:9 */
	{
		DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
			 3192, 3432, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 37 - 2880x576@50Hz 4:3 */
	{
		DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
			 3184, 3456, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 38 - 2880x576@50Hz 16:9 */
	{
		DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
			 3184, 3456, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 39 - 1920x1080i@50Hz 16:9 */
	{
		DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 72000, 1920, 1952,
			 2120, 2304, 0, 1080, 1126, 1136, 1250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 40 - 1920x1080i@100Hz 16:9 */
	{
		DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 41 - 1280x720@100Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
			 1760, 1980, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 42 - 720x576@100Hz 4:3 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 43 - 720x576@100Hz 16:9 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 44 - 720(1440)x576i@100Hz 4:3 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 45 - 720(1440)x576i@100Hz 16:9 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 46 - 1920x1080i@120Hz 16:9 */
	{
		DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
				 DRM_MODE_FLAG_INTERLACE),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 47 - 1280x720@120Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
			 1430, 1650, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 48 - 720x480@120Hz 4:3 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 49 - 720x480@120Hz 16:9 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 50 - 720(1440)x480i@120Hz 4:3 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 51 - 720(1440)x480i@120Hz 16:9 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 52 - 720x576@200Hz 4:3 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 200,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 53 - 720x576@200Hz 16:9 */
	{
		DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732, 796,
			 864, 0, 576, 581, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 200,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 54 - 720(1440)x576i@200Hz 4:3 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 200,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 55 - 720(1440)x576i@200Hz 16:9 */
	{
		DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732, 795,
			 864, 0, 576, 580, 586, 625, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 200,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 56 - 720x480@240Hz 4:3 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 240,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 57 - 720x480@240Hz 16:9 */
	{
		DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736, 798,
			 858, 0, 480, 489, 495, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 240,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 58 - 720(1440)x480i@240Hz 4:3 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 240,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_4_3,
	},
	/* 59 - 720(1440)x480i@240Hz 16:9 */
	{
		DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739, 801,
			 858, 0, 480, 488, 494, 525, 0,
			 DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
				 DRM_MODE_FLAG_INTERLACE |
				 DRM_MODE_FLAG_DBLCLK),
		.vrefresh = 240,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 60 - 1280x720@24Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
			 3080, 3300, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 61 - 1280x720@25Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
			 3740, 3960, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 62 - 1280x720@30Hz 16:9 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
			 3080, 3300, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 63 - 1920x1080@120Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 64 - 1920x1080@100Hz 16:9 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 65 - 1280x720@24Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
			 3080, 3300, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 66 - 1280x720@25Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
			 3740, 3960, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 67 - 1280x720@30Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
			 3080, 3300, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 68 - 1280x720@50Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
			 1760, 1980, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 69 - 1280x720@60Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
			 1430, 1650, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 70 - 1280x720@100Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
			 1760, 1980, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 71 - 1280x720@120Hz 64:27 */
	{
		DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
			 1430, 1650, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 72 - 1920x1080@24Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
			 2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 73 - 1920x1080@25Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 74 - 1920x1080@30Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 75 - 1920x1080@50Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 76 - 1920x1080@60Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 77 - 1920x1080@100Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
			 2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 78 - 1920x1080@120Hz 64:27 */
	{
		DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
			 2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 79 - 1680x720@24Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 3040,
			 3080, 3300, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 80 - 1680x720@25Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2908,
			 2948, 3168, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 81 - 1680x720@30Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2380,
			 2420, 2640, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 82 - 1680x720@50Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 82500, 1680, 1940,
			 1980, 2200, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 83 - 1680x720@60Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 99000, 1680, 1940,
			 1980, 2200, 0, 720, 725, 730, 750, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 84 - 1680x720@100Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 165000, 1680, 1740,
			 1780, 2000, 0, 720, 725, 730, 825, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 85 - 1680x720@120Hz 64:27 */
	{
		DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 198000, 1680, 1740,
			 1780, 2000, 0, 720, 725, 730, 825, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 86 - 2560x1080@24Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 99000, 2560, 3558,
			 3602, 3750, 0, 1080, 1084, 1089, 1100, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 87 - 2560x1080@25Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 90000, 2560, 3008,
			 3052, 3200, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 88 - 2560x1080@30Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 118800, 2560, 3328,
			 3372, 3520, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 89 - 2560x1080@50Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 185625, 2560, 3108,
			 3152, 3300, 0, 1080, 1084, 1089, 1125, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 90 - 2560x1080@60Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 198000, 2560, 2808,
			 2852, 3000, 0, 1080, 1084, 1089, 1100, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 91 - 2560x1080@100Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 371250, 2560, 2778,
			 2822, 2970, 0, 1080, 1084, 1089, 1250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 100,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 92 - 2560x1080@120Hz 64:27 */
	{
		DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 495000, 2560, 3108,
			 3152, 3300, 0, 1080, 1084, 1089, 1250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 120,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 93 - 3840x2160@24Hz 16:9 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
			 5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 94 - 3840x2160@25Hz 16:9 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
			 4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 95 - 3840x2160@30Hz 16:9 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
			 4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 96 - 3840x2160@50Hz 16:9 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
			 4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 97 - 3840x2160@60Hz 16:9 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
			 4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
	},
	/* 98 - 4096x2160@24Hz 256:135 */
	{
		DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5116,
			 5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135,
	},
	/* 99 - 4096x2160@25Hz 256:135 */
	{
		DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5064,
			 5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135,
	},
	/* 100 - 4096x2160@30Hz 256:135 */
	{
		DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 4184,
			 4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135,
	},
	/* 101 - 4096x2160@50Hz 256:135 */
	{
		DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 5064,
			 5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135,
	},
	/* 102 - 4096x2160@60Hz 256:135 */
	{
		DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 4184,
			 4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_256_135,
	},
	/* 103 - 3840x2160@24Hz 64:27 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
			 5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 24,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 104 - 3840x2160@25Hz 64:27 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
			 4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 25,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 105 - 3840x2160@30Hz 64:27 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
			 4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 30,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 106 - 3840x2160@50Hz 64:27 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
			 4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 50,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
	/* 107 - 3840x2160@60Hz 64:27 */
	{
		DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
			 4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
			 DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
		.vrefresh = 60,
		.picture_aspect_ratio = HDMI_PICTURE_ASPECT_64_27,
	},
};

#endif
