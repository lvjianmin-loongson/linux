/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2020 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IT66121_H__
#define __IT66121_H__

#include <linux/hdmi.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

#include <drm/drm_connector.h>
#include <drm/drm_edid.h>

#include "../loongson_bridge.h"

#define IT66121_CHIP_NAME "IT66121"
#define IT66121_CHIP_ADDR 0x4c
#define IT66121_GPIO_PLACEMENT_OFFSET 16U

#define IT66121_INT_CFG IT66121_INT_CFG_HIGH_PUSH
#define IT66121_INT_CFG_LOW_OPEN 0x60
#define IT66121_INT_CFG_LOW_PUSH 0x20
#define IT66121_INT_CFG_HIGH_OPEN 0xE0
#define IT66121_INT_CFG_HIGH_PUSH 0xA0

#define IT66121_REG_START 0x00
#define IT66121_REG_END 0x100
#define IT66121_REG_MAX 0x1BF

#define IT66121_REG_COMM_START 0x00
#define IT66121_REG_COMM_END 0xFF
#define IT66121_REG_HDMI_START 0x130
#define IT66121_REG_HDMI_END 0x1BF

/* General Registers */
#define IT66121_REG_VENDOR_ID_BASE 0x00
#define VENDOR_ID_LEN 2U
#define IT66121_REG_VENDOR_ID(x) (IT66121_REG_VENDOR_ID_BASE + (x))
#define VENDOR_ID0 0U
#define VENDOR_ID1 1U
#define IT66121_REG_DEVICE_ID 0x02
#define IT66121_REG_CHIP_REVISION 0x03

#define IT66121_REG_RESET 0x04
#define IT66121_REG_RESET_HDCP_POS 0U
#define IT66121_REG_RESET_HDCP_MSK BIT_MASK(IT66121_REG_RESET_HDCP_POS)
#define RESET_HDCP 1U
#define IT66121_REG_RESET_HDMI_POS 1U
#define IT66121_REG_RESET_HDMI_MSK BIT_MASK(IT66121_REG_RESET_HDMI_POS)
#define RESET_HDMI 1U
#define IT66121_REG_RESET_AUDIO_FIFO_POS 2U
#define IT66121_REG_RESET_AUDIO_FIFO_MSK                                       \
	BIT_MASK(IT66121_REG_RESET_AUDIO_FIFO_POS)
#define RESET_AUDIO_FIFO 1U
#define IT66121_REG_RESET_VIDEO_POS 3U
#define IT66121_REG_RESET_VIDEO_MSK BIT_MASK(IT66121_REG_RESET_VIDEO_POS)
#define RESET_VIDEO 1U
#define IT66121_REG_RESET_AUDIO_POS 4U
#define IT66121_REG_RESET_AUDIO_MSK BIT_MASK(IT66121_REG_RESET_AUDIO_POS)
#define RESET_AUDIO 1U
#define IT66121_REG_RESET_RCLK_POS 5U
#define IT66121_REG_RESET_RCLK_MSK BIT_MASK(IT66121_REG_RESET_RCLK_POS)
#define RESET_RCLK 1U
#define IT66121_REG_RESET_TEST_POS 7U
#define IT66121_REG_RESET_TEST_MSK BIT_MASK(IT66121_REG_RESET_TEST_POS)
#define RESET_TEST 1U

#define IT66121_REG_CFG 0x05

#define IT66121_REG_INT_STATUS_BASE 0x06
#define IT66121_REG_INT_STATUS_END 0x08
#define INT_STATUS_LEN 3U
#define IT66121_REG_INT_STATUS(x) (IT66121_REG_INT_STATUS_BASE + (x))
#define INT_STATUS0 0U
#define INT_HPD_EVENT_POS 0U
#define INT_HPD_EVENT_MSK BIT_MASK(INT_HPD_EVENT_POS)
#define INT_HPD_EVENT 1U
#define INT_STATUS1 1U
#define INT_STATUS2 2U

#define IT66121_REG_INT_MASK_BASE 0x09
#define IT66121_REG_INT_MASK_END 0x0B
#define INT_MASK_LEN 3U
#define IT66121_REG_INT_MASK(x) (IT66121_REG_INT_MASK_BASE + (x))
#define INT_MASK0 0U
#define INT_MASK0_MSK (0xFF)
#define INT_MASK_HPD_POS 0U
#define INT_MASK_HPD_MSK BIT_MASK(INT_MASK_HPD_POS)
#define INT_MASK_HPD_ENABLE 0U
#define INT_MASK_HPD_DISABLE 1U
#define INT_MASK1 1U
#define INT_MASK1_MSK (0xFF)
#define INT_MASK2 2U
#define INT_MASK2_MSK (0xFF)

#define IT66121_REG_INT_CLEAR_BASE 0x0C
#define IT66121_REG_INT_CLEAR_END 0x0D
#define INT_CLEAR_LEN 2U
#define IT66121_REG_INT_CLEAR(x) (IT66121_REG_INT_CLEAR_BASE + (x))
#define INT_CLEAR0 0U
#define INT_CLEAR0_MSK (0XFF)
#define INT_CLEAR1 1U
#define INT_CLEAR1_MSK (0XFF)
#define IT66121_REG_INT_CLEAR_HPD_POS 0U
#define IT66121_REG_INT_CLEAR_HPD_MSK BIT_MASK(IT66121_REG_INT_CLEAR_HPD_POS)
#define INT_CLEAR_HPD 1U

#define IT66121_REG_SYS_STATUS 0x0E
#define SYS_STATUS_CLEAR_INT_POS 0U
#define SYS_STATUS_CLEAR_INT_MSK BIT_MASK(SYS_STATUS_CLEAR_INT_POS)
#define SYS_CLEAR_INT 1U
#define SYS_STATUS_RX_SEND_POS 5U
#define SYS_STATUS_RX_SEND_MSK BIT_MASK(SYS_STATUS_RX_SEND_POS)
#define SYS_RX_SEND_READY 1U
#define SYS_STATUS_HPD_POS 6U
#define SYS_STATUS_HPD_MSK BIT_MASK(SYS_STATUS_HPD_POS)
#define SYS_HPD_PLUG_OFF 0U
#define SYS_HPD_PLUG_ON 1U
#define SYS_STATUS_INT_ACTIVE_POS 7U
#define SYS_STATUS_INT_ACTIVE_MSK BIT_MASK(SYS_STATUS_INT_ACTIVE_POS)
#define SYS_INT_ACTIVE 1U

#define IT66121_REG_SYS_CLK 0x0F
#define IT66121_REG_PAGE_SELECT IT66121_REG_SYS_CLK
#define IT66121_REG_PAGE_SELECT_POS 0U
#define IT66121_REG_PAGE_SELECT_MSK (0x3 << IT66121_REG_PAGE_SELECT_POS)

#define IT66121_REG_DDC_CTRL 0x10
#define IT66121_REG_DDC_CTRL_MASTER_POS 0U
#define IT66121_REG_DDC_CTRL_MASTER_MSK                                        \
	BIT_MASK(IT66121_REG_DDC_CTRL_MASTER_POS)
#define DDC_CTRL_MASTER_PC 1U
#define DDC_CTRL_MASTER_HDCP 0U
#define IT66121_REG_DDC_CTRL_SW_MODE_POS 3U
#define IT66121_REG_DDC_CTRL_SW_MODE_MSK                                       \
	BIT_MASK(IT66121_REG_DDC_CTRL_SW_MODE_POS)
#define DDC_CTRL_SW_MODE_ENABLE 1U
#define DDC_CTRL_SW_MODE_DISABLE 0U

#define IT66121_REG_DDC_BASE 0x11
#define IT66121_REG_DDC_COUNT 0x5
#define IT66121_REG_DDC_ADDR 0x11
#define IT66121_RX_EDID 0xA0
#define IT66121_RX_HDCP 0x74
#define IT66121_REG_DDC_OFFSET 0x12
#define IT66121_REG_DDC_FIFO_SIZE 0x13
#define IT66121_REG_DDC_EDID_BLOCK 0x14

#define IT66121_REG_DDC_CMD 0x15
#define IT66121_REG_DDC_CMD_POS 0U
#define IT66121_REG_DDC_CMD_MSK (0xF << IT66121_REG_DDC_CMD_POS)
#define DDC_CMD_BURST 0U
#define DDC_CMD_EDID 0x3
#define DDC_CMD_FIFO_CLEAR 0x9
#define DDC_CMD_ABORT 0xF
#define IT66121_REG_DDC_REQ_ROM_SCL_POS 4U
#define IT66121_REG_DDC_REQ_ROM_SCL_MSK                                        \
	BIT_MASK(IT66121_REG_DDC_REQ_ROM_SCL_POS)
#define ROM_SCL_HIGH 1U
#define ROM_SCL_LOW 0U
#define IT66121_REG_DDC_REQ_ROM_SDA_POS 5U
#define IT66121_REG_DDC_REQ_ROM_SDA_MSK                                        \
	BIT_MASK(IT66121_REG_DDC_REQ_ROM_SDA_POS)
#define ROM_SDA_HIGH 1U
#define ROM_SDA_LOW 0U
#define IT66121_REG_DDC_REQ_DDC_SCL_POS 6U
#define IT66121_REG_DDC_REQ_DDC_SCL_MSK                                        \
	BIT_MASK(IT66121_REG_DDC_REQ_DDC_SCL_POS)
#define DDC_SCL_HIGH 1U
#define DDC_SCL_LOW 0U
#define IT66121_REG_DDC_REQ_DDC_SDA_POS 7U
#define IT66121_REG_DDC_REQ_DDC_SDA_MSK                                        \
	BIT_MASK(IT66121_REG_DDC_REQ_DDC_SDA_POS)
#define DDC_SDA_HIGH 1U
#define DDC_SDA_LOW 0U

#define IT66121_REG_DDC_STATUS 0x16
#define IT66121_REG_DDC_STATUS_FIFO_TX_POS 0U
#define IT66121_REG_DDC_STATUS_FIFO_TX_MSK                                     \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_TX_POS)
#define DDC_STATUS_FIFO_TX_VALID 1U
#define IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS 1U
#define IT66121_REG_DDC_STATUS_FIFO_EMPTY_MSK                                  \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS)
#define DDC_STATUS_FIFO_EMPTY 1U
#define IT66121_REG_DDC_STATUS_FIFO_FULL_POS 2U
#define IT66121_REG_DDC_STATUS_FIFO_FULL_MSK                                   \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_FULL_POS)
#define DDC_STATUS_FIFO_FULL 1U
#define IT66121_REG_DDC_STATUS_FIFO_E_LOSE_POS 3U
#define IT66121_REG_DDC_STATUS_FIFO_E_LOSE_MSK                                 \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_E_LOSE_POS)
#define DDC_STATUS_FIFO_E_LOSE 1U
#define IT66121_REG_DDC_STATUS_FIFO_E_WAIT_POS 4U
#define IT66121_REG_DDC_STATUS_FIFO_E_WAIT_MSK                                 \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_E_WAIT_POS)
#define DDC_STATUS_FIFO_E_WAIT 1U
#define IT66121_REG_DDC_STATUS_FIFO_E_NACK_POS 5U
#define IT66121_REG_DDC_STATUS_FIFO_E_NACK_MSK                                 \
	BIT_MASK(IT66121_REG_DDC_STATUS_FIFO_E_NACK_POS)
#define DDC_STATUS_FIFO_E_NACK 1U
#define IT66121_REG_DDC_STATUS_RX_ACTIVE_POS 6U
#define IT66121_REG_DDC_STATUS_RX_ACTIVE_MSK                                   \
	BIT_MASK(IT66121_REG_DDC_STATUS_RX_ACTIVE_POS)
#define DDC_STATUS_RX_ACTIVE 1U
#define IT66121_REG_DDC_STATUS_RX_COMPLETE_POS 7U
#define IT66121_REG_DDC_STATUS_RX_COMPLETE_MSK                                 \
	BIT_MASK(IT66121_REG_DDC_STATUS_RX_COMPLETE_POS)
#define DDC_STATUS_RX_COMPLETE 1U

#define IT66121_REG_FIFO_CONTENT 0x17
#define IT66121_FIFO_MAX_LENGTH 32U

#define IT66121_REG_INPUT_DATA 0x70
#define IT66121_REG_INPUT_DATA_SIGNAL_TYPE_POS 2U
#define IT66121_REG_INPUT_DATA_SIGNAL_TYPE_MSK                                 \
	BIT_MASK(IT66121_REG_INPUT_SIGNAL_TYPE_POS)
#define INPUT_SDR 0U
#define INPUT_DDR 1U
#define IT66121_REG_INPUT_DATA_COLOR_SPACE_POS 6U
#define IT66121_REG_INPUT_DATA_COLOR_SPACE_MSK                                 \
	(0x3 << IT66121_REG_INPUT_DATA_COLOR_SPACE_POS)
#define INPUT_RGB 0U
#define INPUT_YUV_422 1U
#define INPUT_YUV_444 0x2

#define IT66121_REG_HDMI_MODE 0xC0
#define IT66121_REG_HDMI_MODE_POS 0U
#define IT66121_REG_HDMI_MODE_MSK BIT_MASK(IT66121_REG_HDMI_MODE_POS)
#define DVI_MODE 0U
#define HDMI_MODE 1U

#define IT66121_REG_AVMUTE 0xC1
#define IT66121_REG_AVMUTE_POS 0U
#define IT66121_REG_AVMUTE_MSK BIT_MASK(IT66121_REG_AVMUTE_POS)
#define AVMUTE_OFF 0U
#define AVMUTE_ON 1U
#define IT66121_REG_AVMUTE_COLOR_POS 1U
#define IT66121_REG_AVMUTE_COLOR_MSK BIT_MASK(IT66121_REG_AVMUTE_COLOR_POS)
#define AVMUTE_COLOR_BLACK 0U
#define AVMUTE_COLOR_BLUE 1U
#define IT66121_REG_AVMUTE_STATUS_POS 0U
#define IT66121_REG_AVMUTE_STATUS_MSK BIT_MASK(IT66121_REG_AVMUTE_STATUS_POS)
#define AVMUTE_STATUS_OFF 0U
#define AVMUTE_STATUS_ON 1U

#define IT66121_REG_CSC_UPPER(x) (0x18 + (x)*2)
#define IT66121_REG_CSC_LOWER(x) (0x19 + (x)*2)
#define IT66121_REG_DE_GENERATOR (0x35 + (x))

#define IT66121_REG_AVI_BASE 0x158
#define AVI_LENGTH 14U
#define IT66121_REG_AVI(x) (IT66121_REG_AVI_BASE + (x))
#define AVI_PB1 0U /* 0x158 */
#define AVI_PB2 1U /* 0x159 */
#define AVI_PB3 2U /* 0x15A */
#define AVI_PB4 3U /* 0x15B */
#define AVI_PB5 4U /* 0x15C */
#define AVI_SUM 5U /* 0x15D */
#define AVI_PB6 6U /* 0x15E */
#define AVI_PB7 7U /* 0x15F */
#define AVI_PB8 8U /* 0x160 */
#define AVI_PB9 9U /* 0x161 */
#define AVI_PB10 10U /* 0x162 */
#define AVI_PB11 11U /* 0x163 */
#define AVI_PB12 12U /* 0x164 */
#define AVI_PB13 13U /* 0x165 */
#define AVI_BLOCK_0_BASE IT66121_REG_AVI(AVI_PB1)
#define AVI_BLOCK_0_LENGTH 5U
#define AVI_BLOCK_SUM IT66121_REG_AVI(AVI_SUM)
#define AVI_BLOCK_1_BASE IT66121_REG_AVI(AVI_PB6)
#define AVI_BLOCK_1_LENGTH 8U

struct it66121;

struct reg_mask_seq {
	unsigned int reg;
	unsigned int mask;
	unsigned int val;
};

enum signal_type {
	SDR_CLOCK,
	DDR_CLOCK,
};

enum latch_clock {
	FULL_PERIOD,
	HALF_PERIOD,
};

enum color_space_convert {
	csc_NONE,
	csc_RGB2YUV,
	csc_YUV2RGB,
};

enum it66121_hpd_status {
	hpd_status_plug_off = 0,
	hpd_status_plug_on = 1,
};

struct it66121_ddc_status {
	struct mutex ddc_bus_mutex;
	bool ddc_bus_idle;
	bool ddc_fifo_empty;
};

struct it66121_mode_config {
	bool edid_read;
	u8 edid_buf[256];

	bool hdmi_mode;
	union hdmi_infoframe hdmi_frame;

	struct {
		bool use_ccir656;
		enum hdmi_colorspace input_colorspace;
		enum color_space_convert it66121_csc;
		int pclk_delay;
		enum latch_clock pclk_latch;
		enum signal_type input_signal_sample_type;
		struct drm_display_mode mode;
		bool gen_sync;
	} it66121_input_mode;

	struct {
	} it66121_output_mode;
};

struct it66121_config_funcs {
	int (*reg_init)(struct it66121 *it66121_dev);
	int (*power_on)(struct it66121 *it66121_dev);
	int (*power_off)(struct it66121 *it66121_dev);
	int (*set_afe_high)(struct it66121 *it66121_dev);
	int (*set_afe_low)(struct it66121 *it66121_dev);
	int (*set_dvi_mode)(struct it66121 *it66121_dev);
	int (*set_hdmi_mode)(struct it66121 *it66121_dev);
	int (*hdcp_enable)(struct it66121 *it66121_dev);
	int (*debugfs_init)(struct it66121 *it66121_dev);
};

struct it66121 {
	struct bridge_resource res;

	struct loongson_device *ldev;

	struct loongson_i2c *li2c;
	struct i2c_client *i2c_it66121;
	struct i2c_client *i2c_edid;
	struct i2c_client *i2c_cec;

	struct regmap *it66121_regmap;
	const struct it66121_config_funcs *funcs;

	int irq;
	atomic_t irq_status;
	u8 sys_status;
	enum it66121_hpd_status hpd_status;

	struct it66121_ddc_status ddc_status;
	u8 edid_buf[EDID_LENGTH];
	u8 fifo_buf[IT66121_FIFO_MAX_LENGTH];

	struct it66121_mode_config mode_config;

	struct drm_bridge bridge;
	struct drm_encoder *encoder;
	struct drm_connector connector;
	enum drm_connector_status status;
	const struct drm_display_mode *duplicated_mode;
};

enum ddc_ctrl_master {
	MASTER_HDCP = 0,
	MASTER_PC,
};

enum ddc_cmd {
	CMD_BURST = 0x0,
	CMD_EDID = 0x3,
	CMD_FIFO_CLEAR = 0x9,
	CMD_ABORT = 0xF,
};

#define bridge_to_it66121(drm_bridge)                                          \
	container_of(drm_bridge, struct it66121, bridge)
#define connector_to_it66121(drm_connector)                                    \
	container_of(drm_connector, struct it66121, connector)
#define to_dev(it66121) (&it66121->i2c_it66121->dev)

int loongson_it66121_init(struct bridge_resource *res);
int it66121_remove(struct it66121 *it66121_dev);

#endif
