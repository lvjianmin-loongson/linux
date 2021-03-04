// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/gpio.h>

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include "it66121_drv.h"
#include "it66121_regs.h"
#include "../loongson_drv.h"
#include "../loongson_bridge.h"

#define DEBUG_MSEEAGE 0
/**
 * @section IT66121 helper functions
 */
static void __it66121_write_reg_mask_seq(struct it66121 *it66121_dev,
					 const struct reg_mask_seq *seq,
					 size_t seq_size)
{
	unsigned int i;
	struct regmap *regmap;

	regmap = it66121_dev->it66121_regmap;
	for (i = 0; i < seq_size; i++)
		regmap_update_bits(regmap, seq[i].reg, seq[i].mask, seq[i].val);
}

static int it66121_reg_dump(struct it66121 *it66121_dev, size_t start,
			    size_t count)
{
	u8 *buf;
	int ret;
	unsigned int i;

	buf = kzalloc(count, GFP_KERNEL);
	if (IS_ERR(buf)) {
		ret = PTR_ERR(buf);
		return -ENOMEM;
	}
	ret = regmap_raw_read(it66121_dev->it66121_regmap, start, buf, count);
	for (i = 0; i < count; i++)
		pr_info("[%lx]=%x", start + i, buf[i]);

	kfree(buf);
	return ret;
}

static int it66121_reg_dump_all(struct it66121 *it66121_dev)
{
	unsigned int ret;

	DRM_INFO("Dump it66121 registers:\n");
	ret = it66121_reg_dump(it66121_dev, IT66121_REG_START, IT66121_REG_MAX);
	return ret;
}

#define SIMPLE_REG_SEQ_FUNC(CMD) it66121_##CMD

#define DEFINE_SIMPLE_REG_SEQ_FUNC(CMD)                                        \
	static int it66121_##CMD(struct it66121 *it66121_dev)                  \
	{                                                                      \
		regmap_multi_reg_write(it66121_dev->it66121_regmap,            \
				       it66121_##CMD##_seq,                    \
				       ARRAY_SIZE(it66121_##CMD##_seq));       \
		return 0;                                                      \
	}

DEFINE_SIMPLE_REG_SEQ_FUNC(reg_init)
DEFINE_SIMPLE_REG_SEQ_FUNC(power_on)
DEFINE_SIMPLE_REG_SEQ_FUNC(power_off)
DEFINE_SIMPLE_REG_SEQ_FUNC(afe_high)
DEFINE_SIMPLE_REG_SEQ_FUNC(afe_low)
DEFINE_SIMPLE_REG_SEQ_FUNC(hdmi)

static void it66121_hdcp_enable(struct it66121 *it66121_dev)
{
	__it66121_write_reg_mask_seq(it66121_dev, it66121_hdcp_seq,
				     ARRAY_SIZE(it66121_hdcp_seq));
}
static void it66121_get_status(struct it66121 *it66121_dev)
{
	struct regmap *regmap;
	unsigned int sys, clk;

	regmap = it66121_dev->it66121_regmap;
	regmap_read(regmap, IT66121_REG_SYS_STATUS, &sys);
	regmap_read(regmap, IT66121_REG_SYS_CLK, &clk);

	DRM_DEV_DEBUG(to_dev(it66121_dev), "[SYS]=0x%x, [CLK]=0x%x\n", sys,
		      clk);
}

static int it66121_chip_id_verify(struct it66121 *it66121_dev)
{
	int ret;
	u8 buf[4];

	struct regmap *regmap = it66121_dev->it66121_regmap;

	ret = regmap_raw_read(regmap, IT66121_REG_VENDOR_ID_BASE, &buf,
			      ARRAY_SIZE(buf));
	if (ret) {
		DRM_DEV_ERROR(to_dev(it66121_dev), "Failed to get chip id\n");
		return -EIO;
	}

	DRM_DEV_DEBUG(to_dev(it66121_dev), "Encoder vendor: %c%c\n", buf[1],
		      buf[0]);
	return ret;
}

static enum drm_mode_status it66121_mode_valid(struct it66121 *it66121_dev,
					       struct drm_display_mode *mode)
{
	return MODE_OK;
}

static void it66121_mode_set(struct it66121 *it66121_dev,
			     struct drm_display_mode *mode,
			     struct drm_display_mode *adj_mode)
{
}

const char *it66121_get_hpd_status_name(enum it66121_hpd_status status)
{
	if (status == hpd_status_plug_on)
		return "Plug on";
	else if (status == hpd_status_plug_off)
		return "Plug off";
	else
		return "Unknown";
}

static int it66121_debugfs_init(struct it66121 *it66121_dev);

static const struct it66121_config_funcs it66121_cfg_funcs = {
	.reg_init = SIMPLE_REG_SEQ_FUNC(reg_init),
	.power_on = SIMPLE_REG_SEQ_FUNC(power_on),
	.power_off = SIMPLE_REG_SEQ_FUNC(power_off),
	.set_afe_high = SIMPLE_REG_SEQ_FUNC(afe_high),
	.set_afe_low = SIMPLE_REG_SEQ_FUNC(afe_low),
	.set_hdmi_mode = SIMPLE_REG_SEQ_FUNC(hdmi),
	.debugfs_init = it66121_debugfs_init,
};

/**
 * @section IT66121 INT and HPD functions
 */
static enum it66121_hpd_status
it66121_get_hpd_status(struct it66121 *it66121_dev)
{
	unsigned int val;

	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_SYS_STATUS, &val);
	if (test_bit(SYS_STATUS_HPD_POS, (unsigned long *)&val) ==
	    SYS_HPD_PLUG_ON) {
		DRM_DEV_DEBUG(to_dev(it66121_dev),
			      "IT66121 hpd status: Connected\n");
		return hpd_status_plug_on;
	}

	DRM_DEV_DEBUG(to_dev(it66121_dev),
		      "IT66121 hpd status: Disconnected\n");
	return hpd_status_plug_off;
}

static int it66121_int_enable_hpd(struct it66121 *it66121_dev)
{
	int ret;

	ret = regmap_update_bits(it66121_dev->it66121_regmap,
				 IT66121_REG_INT_MASK(INT_MASK0),
				 INT_MASK_HPD_MSK, INT_MASK_HPD_ENABLE);
	if (ret) {
		DRM_ERROR("Failed to enable hpd interrupt, ret=%d\n", ret);
		return -EIO;
	}
	it66121_get_status(it66121_dev);
	return 0;
}

static int it66121_int_disable_hpd(struct it66121 *it66121_dev)
{
	int ret;

	ret = regmap_update_bits(it66121_dev->it66121_regmap,
				 IT66121_REG_INT_MASK(INT_MASK0),
				 INT_MASK_HPD_MSK, INT_MASK_HPD_DISABLE);
	if (ret) {
		DRM_ERROR("Failed to enable hpd interrupt, ret=%d\n", ret);
		return -EIO;
	}
	return 0;
}

static int it66121_int_enable_all(struct it66121 *it66121_dev)
{
	regmap_multi_reg_write(it66121_dev->it66121_regmap, it66121_int_enable,
			       ARRAY_SIZE(it66121_int_enable));
	return 0;
}

static int it66121_int_disable_all(struct it66121 *it66121_dev)
{
	regmap_multi_reg_write(it66121_dev->it66121_regmap, it66121_int_disable,
			       ARRAY_SIZE(it66121_int_disable));
	return 0;
}

static int __do_clear_int(struct it66121 *it66121_dev)
{
	int ret;
	unsigned int val;

	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_SYS_STATUS, &val);
	ret = regmap_update_bits(it66121_dev->it66121_regmap,
				 IT66121_REG_SYS_STATUS,
				 SYS_STATUS_CLEAR_INT_MSK, SYS_CLEAR_INT);
	if (ret) {
		DRM_ERROR("Failed to do clear INT\n");
		return -EIO;
	}
	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_SYS_STATUS, &val);
	DRM_DEV_DEBUG(to_dev(it66121_dev), "Do clear INT done [0E]=%x\n", val);
	return 0;
}

static int it66121_int_clear(struct it66121 *it66121_dev, unsigned int num,
			     unsigned int mask)
{
	int ret;

	ret = regmap_update_bits(it66121_dev->it66121_regmap,
				 IT66121_REG_INT_CLEAR(num), mask, 1U);
	if (ret) {
		DRM_ERROR("Failed to clear interrupt [%x-%x], ret=%d\n", num,
			  mask, ret);
		return -EIO;
	}
	__do_clear_int(it66121_dev);
	return 0;
}

static void it66121_int_clear_all(struct it66121 *it66121_dev)
{
	struct regmap *it66121_regmap;

	it66121_regmap = it66121_dev->it66121_regmap;
	DRM_DEV_DEBUG(to_dev(it66121_dev), "Clear all interrupts\n");

	regmap_write(it66121_regmap, IT66121_REG_INT_CLEAR(INT_CLEAR0),
		     INT_CLEAR0_MSK);
	regmap_write(it66121_regmap, IT66121_REG_INT_CLEAR(INT_CLEAR1),
		     INT_CLEAR1_MSK);
	__do_clear_int(it66121_dev);
}

static irqreturn_t it66121_isr_thread(int irq_num, void *devid)
{
	int ret;
	u8 irq[3];
	unsigned int sys, clk;
	bool hpd_event, hpd_status, interrupt_status;
	struct device *dev;
	struct regmap *regmap;
	struct it66121 *it66121_dev;

	it66121_dev = (struct it66121 *)devid;
	dev = to_dev(it66121_dev);
	regmap = it66121_dev->it66121_regmap;
	DRM_DEV_DEBUG(dev, "IT66121 isr thread\n");

	ret = regmap_read(regmap, IT66121_REG_SYS_CLK, &clk);
	if (ret < 0)
		goto error_io;
	ret = regmap_read(regmap, IT66121_REG_SYS_STATUS, &sys);
	if (ret < 0)
		goto error_io;
	interrupt_status =
		test_bit(SYS_STATUS_INT_ACTIVE_POS, (unsigned long *)&sys);
	if (interrupt_status != SYS_INT_ACTIVE) {
		DRM_DEV_DEBUG(dev, "Not IT66121 interrupt [0E]=%x\n", sys);

		return IRQ_NONE;
	}
	DRM_DEV_DEBUG(dev, "IT66121 interrupt active: [sys]=%x,[clk]=%x\n", sys,
		      clk);

	regmap_bulk_read(regmap, IT66121_REG_INT_STATUS_BASE, irq,
			 INT_STATUS_LEN);
	DRM_DEV_DEBUG(dev, "IT66121 interrupt status: {%x,%x,%x}\n", irq[0],
		      irq[1], irq[2]);

	hpd_event = test_bit(INT_HPD_EVENT_POS, (unsigned long *)&irq[0]);
	hpd_status = test_bit(SYS_STATUS_HPD_POS, (unsigned long *)&sys);

	if (hpd_event == INT_HPD_EVENT) {
		DRM_DEV_INFO(dev, "IT66121 HPD event [CONNECTOR:%d %s %s]\n",
			     it66121_dev->connector.base.id,
			     it66121_dev->connector.name,
			     it66121_get_hpd_status_name(hpd_status));

		it66121_int_clear(it66121_dev, INT_CLEAR0, INT_CLEAR_HPD);
		it66121_int_clear_all(it66121_dev);
		it66121_dev->hpd_status = hpd_status;
		drm_kms_helper_hotplug_event(it66121_dev->connector.dev);
	} else {
		DRM_DEV_DEBUG(dev, "Not HPD event\n");
		it66121_int_clear_all(it66121_dev);
	}

	return IRQ_HANDLED;
error_io:
	DRM_DEV_ERROR(to_dev(it66121_dev),
		      "Failed to read IT66121 registers\n");

	return IRQ_NONE;
}

/**
 * @section IT66121 DDC functions
 */
static void __it66121_ddc_cmd(struct it66121 *it66121_dev, enum ddc_cmd cmd)
{
	regmap_update_bits(it66121_dev->it66121_regmap, IT66121_REG_DDC_CMD,
			   IT66121_REG_DDC_CMD_MSK, cmd);
}

static int __it66121_ddc_select_master(struct it66121 *it66121_dev,
				       enum ddc_ctrl_master master)
{
	unsigned int val;

	regmap_update_bits(it66121_dev->it66121_regmap, IT66121_REG_DDC_CTRL,
			   IT66121_REG_DDC_CTRL_MASTER_MSK, master);
	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_DDC_CTRL, &val);
	if ((val & IT66121_REG_DDC_CTRL_MASTER_MSK) == 0) {
		DRM_DEV_ERROR(to_dev(it66121_dev),
			      "Failed set master PC [10]=%x\n", val);
	}

	return 0;
}

static int it66121_ddc_hdcp_reset(struct it66121 *it66121_dev)
{
	int ret;

	ret = regmap_update_bits(it66121_dev->it66121_regmap, IT66121_REG_RESET,
				 IT66121_REG_RESET_HDCP_MSK, RESET_HDCP);
	return ret;
}

static void it66121_ddc_fifo_abort(struct it66121 *it66121_dev)
{
	__it66121_ddc_cmd(it66121_dev, CMD_ABORT);
}

static int it66121_ddc_fifo_clear(struct it66121 *it66121_dev)
{
	unsigned int val;
	bool fifo_status;

	__it66121_ddc_select_master(it66121_dev, MASTER_PC);
	__it66121_ddc_cmd(it66121_dev, CMD_ABORT);
	__it66121_ddc_cmd(it66121_dev, CMD_FIFO_CLEAR);
	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_DDC_STATUS, &val);
	DRM_DEV_DEBUG(to_dev(it66121_dev), "Cleared FIFO, status=%x\n", val);
	fifo_status = test_bit(IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS,
			       (unsigned long *)&val);
	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_FIFO_CONTENT,
		    &val);
	if (fifo_status != DDC_STATUS_FIFO_EMPTY) {
		DRM_DEV_ERROR(
			to_dev(it66121_dev),
			"Failed to clear DDC FIFO, fifo_status=%d, fifo_content=%x\n",
			fifo_status, val);
		return -EIO;
	}

	DRM_DEV_DEBUG(to_dev(it66121_dev), "DDC FIFO is empty\n");
	return 0;
}

static int it66121_ddc_fifo_fetch(struct it66121 *it66121_dev, u8 *buf,
				  unsigned int block, size_t len, size_t offset)
{
	u8 *pfifo;
	struct regmap *regmap;
	unsigned int val, i, retry;
	bool ddc_bus_idle, ddc_fifo_empty;
	unsigned long time_s, time_e;
	u8 regs[] = { IT66121_RX_EDID, offset, len, block };
#if (DEBUG_MSEEAGE)
	u8 cfg_regs[7] = {};
#endif

	pfifo = buf;
	regmap = it66121_dev->it66121_regmap;

	if (len > IT66121_FIFO_MAX_LENGTH) {
		DRM_ERROR("[EDID]: Failed to fetch FIFO %ldB overflow\n", len);
		return -EOVERFLOW;
	}
	it66121_dev->hpd_status = it66121_get_hpd_status(it66121_dev);
	if (it66121_dev->hpd_status != hpd_status_plug_on) {
		DRM_ERROR("[EDID]: Failed to fetch FIFO, connector plug off\n");
		return -ENODEV;
	}

	regmap_read(it66121_dev->it66121_regmap, IT66121_REG_DDC_STATUS, &val);
	if (test_bit(IT66121_REG_DDC_STATUS_RX_COMPLETE_POS,
		     (unsigned long *)&val)) {
		it66121_dev->ddc_status.ddc_bus_idle = 1;
		DRM_DEV_DEBUG(to_dev(it66121_dev), "DDC ready, status=%x\n",
			      val);
	} else {
		DRM_DEV_ERROR(to_dev(it66121_dev),
			      "DDC bus not ready, abort\n");
		it66121_ddc_fifo_abort(it66121_dev);
		return -EBUSY;
	}
	if (test_bit(IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS,
		     (unsigned long *)&val))
		it66121_dev->ddc_status.ddc_fifo_empty = 1;
	else {
		DRM_DEV_DEBUG(to_dev(it66121_dev),
			      "DDC FIFO not empty, clear\n");
		it66121_ddc_fifo_clear(it66121_dev);
	}
	it66121_ddc_fifo_clear(it66121_dev);

	__it66121_ddc_select_master(it66121_dev, MASTER_PC);
	regmap_bulk_write(regmap, IT66121_REG_DDC_BASE, regs, ARRAY_SIZE(regs));
#if (DEBUG_MSEEAGE)
	regmap_bulk_read(regmap, IT66121_REG_DDC_BASE - 1, cfg_regs,
			 ARRAY_SIZE(cfg_regs));
	DRM_DEV_DEBUG(to_dev(it66121_dev), "cfg_regs: %x %x %x %x %x %x %x\n",
		      cfg_regs[0], cfg_regs[1], cfg_regs[2], cfg_regs[3],
		      cfg_regs[4], cfg_regs[5], cfg_regs[6]);
#endif
	__it66121_ddc_cmd(it66121_dev, CMD_EDID);

	time_s = jiffies;
	ddc_bus_idle = ddc_fifo_empty = 0;
	for (retry = 0; retry < 5; retry++) {
		schedule_timeout(msecs_to_jiffies(1));
		regmap_read(regmap, IT66121_REG_DDC_STATUS, &val);
		ddc_bus_idle = test_bit(IT66121_REG_DDC_STATUS_RX_COMPLETE_POS,
					(unsigned long *)&val);
		ddc_fifo_empty = test_bit(IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS,
					  (unsigned long *)&val);
		if ((ddc_bus_idle && !ddc_fifo_empty))
			break;
		DRM_DEBUG("FIFO fetch not complete, status=%x\n", val);
	}
	time_e = jiffies;
	DRM_DEV_DEBUG(to_dev(it66121_dev),
		      "FIFO fetch completed, status=%x %dms\n", val,
		      jiffies_to_msecs(time_e - time_s));

	for (i = 0; i < len; i++, pfifo++) {
		regmap_read(regmap, IT66121_REG_FIFO_CONTENT,
			    (unsigned int *)pfifo);
#if (DEBUG_MSEEAGE)
		DRM_DEBUG("Get FIFO context[%d] %x\n", i, *pfifo);
		regmap_read(regmap, IT66121_REG_DDC_STATUS, &val);
		if (test_bit(IT66121_REG_DDC_STATUS_FIFO_EMPTY_POS,
			     (unsigned long *)&val) == DDC_STATUS_FIFO_EMPTY) {
			DRM_DEBUG("FIFO is empty, read back %dB\n", i);
			break;
		}
#endif
	}
	return 0;
}

static int it66121_get_edid_block(void *data, u8 *buf, unsigned int block,
				  size_t len)
{
	int ret;
	unsigned int i, batch_num;
	struct it66121 *it66121_dev;
	u8 *pfifo, *pbuf;

	it66121_dev = data;
	pfifo = it66121_dev->fifo_buf;
	pbuf = it66121_dev->edid_buf;
	batch_num = len / IT66121_FIFO_MAX_LENGTH;
	DRM_DEV_DEBUG(to_dev(it66121_dev),
		      "[EDID]: Request block%d-%ldB, divided %d batches\n",
		      block, len, batch_num);
	if (len > EDID_LENGTH) {
		ret = -EOVERFLOW;
		DRM_ERROR("[EDID]: Failed to request %ldB overflow\n", len);
		return ret;
	}
	for (i = 0; i < batch_num; i++) {
		ret = it66121_ddc_fifo_fetch(it66121_dev, pfifo, block,
					     IT66121_FIFO_MAX_LENGTH,
					     i * IT66121_FIFO_MAX_LENGTH);

		if (ret) {
			DRM_DEV_ERROR(
				to_dev(it66121_dev),
				"[EDID]: Failed to fetched FIFO batches[%d/%d], ret=%d\n",
				i + 1, batch_num, ret);

			return -EIO;
		}
		DRM_DEV_DEBUG(to_dev(it66121_dev),
			      "[EDID]: Fetched batches[%d/%d] %dByete\n", i + 1,
			      batch_num, IT66121_FIFO_MAX_LENGTH);
		memcpy(pbuf, pfifo, IT66121_FIFO_MAX_LENGTH);
		pbuf += IT66121_FIFO_MAX_LENGTH;
	}
	memcpy(buf, it66121_dev->edid_buf, len);
	return 0;
}

static int it66121_get_modes(struct it66121 *it66121_dev,
			     struct drm_connector *connector)
{
	struct edid *edid;
	unsigned int count;

	edid = drm_do_get_edid(connector, it66121_get_edid_block, it66121_dev);
	if (edid) {
		drm_connector_update_edid_property(connector, edid);
		count = drm_add_edid_modes(connector, edid);
		DRM_DEV_DEBUG(to_dev(it66121_dev), "Update %d edid method\n",
			      count);
	} else {
		drm_add_modes_noedid(connector, 1920, 1080);
		count = 0;
		DRM_DEV_INFO(to_dev(it66121_dev),
			     "Update default edid method 1080p\n");
		return count;
	}
	kfree(edid);

	return count;
}

/**
 * @section IT66121 AVI Infoframe functions
 */

static int it66121_set_avi_infoframe(struct it66121 *it66121_dev,
				     struct drm_display_mode *mode)
{
	u8 *ptr, buf[HDMI_INFOFRAME_SIZE(AVI)];
	struct regmap *regmap;
	union hdmi_infoframe *hdmi_frame;
	struct hdmi_avi_infoframe *avi_frame;

	regmap = it66121_dev->it66121_regmap;
	hdmi_frame = &it66121_dev->mode_config.hdmi_frame;
	avi_frame = &hdmi_frame->avi;
	hdmi_avi_infoframe_init(avi_frame);
	drm_hdmi_avi_infoframe_from_display_mode(avi_frame, mode, false);

	hdmi_infoframe_log(KERN_DEBUG, to_dev(it66121_dev), hdmi_frame);
	hdmi_avi_infoframe_pack(avi_frame, buf, sizeof(buf));
	ptr = buf + HDMI_INFOFRAME_HEADER_SIZE;
#if (DEBUG_MSEEAGE)
	DRM_DEBUG("Set AVI reg block-0 0x%x size %d\n", AVI_BLOCK_0_BASE,
		  AVI_BLOCK_0_LENGTH);
	DRM_DEBUG("%x %x %x %x %x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
	DRM_DEBUG("Set AVI reg checksum 0x%x=%x\n", AVI_BLOCK_SUM, buf[3]);
#endif
	regmap_bulk_write(regmap, AVI_BLOCK_0_BASE, ptr, AVI_BLOCK_0_LENGTH);
	regmap_write(regmap, AVI_BLOCK_SUM, buf[3]);
	ptr += AVI_BLOCK_0_LENGTH;
	regmap_bulk_write(regmap, AVI_BLOCK_1_BASE, ptr, AVI_BLOCK_1_LENGTH);
#if (DEBUG_MSEEAGE)
	DRM_DEBUG("Set AVI reg block-1 0x%x size %d\n", AVI_BLOCK_1_BASE,
		  AVI_BLOCK_1_LENGTH);
	DRM_DEBUG("%x %x %x %x %x %x %x %x\n", ptr[0], ptr[1], ptr[2], ptr[3],
		  ptr[4], ptr[5], ptr[6], ptr[7]);
#endif

	return 0;
}

/**
 * @section DRM connector functions
 */
static int it66121_connector_get_modes(struct drm_connector *connector)
{
	struct it66121 *it66121_dev = connector_to_it66121(connector);

	return it66121_get_modes(it66121_dev, connector);
}

static enum drm_mode_status
it66121_connector_mode_valid(struct drm_connector *connector,
			     struct drm_display_mode *mode)
{
	struct it66121 *it66121_dev = connector_to_it66121(connector);

	return it66121_mode_valid(it66121_dev, mode);
}

static struct drm_encoder *
it66121_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id;

	enc_id = connector->encoder_ids[0];
	/* pick the encoder ids */
	if (enc_id)
		return drm_encoder_find(connector->dev, NULL, enc_id);
	return NULL;
}

static struct drm_connector_helper_funcs it66121_connector_helper_funcs = {
	.get_modes = it66121_connector_get_modes,
	.mode_valid = it66121_connector_mode_valid,
	.best_encoder = it66121_connector_best_encoder,
};

static enum drm_connector_status
it66121_connector_detect(struct drm_connector *connector, bool force)
{
	enum drm_connector_status status;
	struct it66121 *it66121_dev;

	it66121_dev = connector_to_it66121(connector);
	status = it66121_get_hpd_status(it66121_dev) ?
			       connector_status_connected :
			       connector_status_disconnected;
	DRM_DEV_INFO(to_dev(it66121_dev),
		     "IT66121 detect: [CONNECTOR:%d:%s] %s\n",
		     connector->base.id, connector->name,
		     drm_get_connector_status_name(status));

	return status;
}

static void it66121_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
	kfree(connector);
}

static const struct drm_connector_funcs it66121_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = it66121_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = it66121_connector_destroy,
};

/**
 * @section DRM bridge functions
 */
static void it66121_bridge_enable(struct drm_bridge *bridge)
{
	struct it66121 *it66121_dev = bridge_to_it66121(bridge);

	DRM_DEBUG("[IT66121] Bridge enable\n");
	it66121_power_on(it66121_dev);
}

static void it66121_bridge_disable(struct drm_bridge *bridge)
{
	struct it66121 *it66121_dev = bridge_to_it66121(bridge);

	DRM_DEBUG("[IT66121] Bridge disable\n");
	it66121_power_off(it66121_dev);
}

static void it66121_bridge_mode_set(struct drm_bridge *bridge,
				    struct drm_display_mode *mode,
				    struct drm_display_mode *adj_mode)
{
	struct it66121 *it66121_dev = bridge_to_it66121(bridge);

	DRM_DEBUG("[IT66121] Bridge mode set\n");
	if (it66121_dev->mode_config.it66121_input_mode.gen_sync)
		DRM_DEV_ERROR(bridge->dev->dev, "it66121 gen_sync\n");
	DRM_DEV_DEBUG(to_dev(it66121_dev), "Modeline " DRM_MODE_FMT "\n",
		      DRM_MODE_ARG(mode));

	it66121_set_avi_infoframe(it66121_dev, mode);
	it66121_mode_set(it66121_dev, mode, adj_mode);
}

static int it66121_bridge_attach(struct drm_bridge *bridge)
{
	struct it66121 *it66121_dev = bridge_to_it66121(bridge);
	int ret;

	DRM_DEV_DEBUG(to_dev(it66121_dev), "IT66121: bridge attach\n");
	if (!bridge->encoder) {
		DRM_ERROR("Parent encoder object not found\n");
		return -ENODEV;
	}

	if (it66121_dev->irq)
		it66121_dev->connector.polled = DRM_CONNECTOR_POLL_HPD;
	else
		it66121_dev->connector.polled = DRM_CONNECTOR_POLL_CONNECT |
						DRM_CONNECTOR_POLL_DISCONNECT;
	DRM_DEV_DEBUG(to_dev(it66121_dev), "IT66121: Set connnector poll=%d\n",
		      it66121_dev->connector.polled);

	ret = drm_connector_init(bridge->dev, &it66121_dev->connector,
				 &it66121_connector_funcs,
				 DRM_MODE_CONNECTOR_HDMIA);
	if (ret) {
		DRM_ERROR("Failed to initialize connector with drm\n");
		return ret;
	}
	drm_connector_helper_add(&it66121_dev->connector,
				 &it66121_connector_helper_funcs);
	drm_connector_attach_encoder(&it66121_dev->connector, bridge->encoder);
	return ret;
}

static const struct drm_bridge_funcs it66121_bridge_funcs = {
	.enable = it66121_bridge_enable,
	.disable = it66121_bridge_disable,
	.mode_set = it66121_bridge_mode_set,
	.attach = it66121_bridge_attach,
};

/**
 * @section IT66121 Regmap amd device attribute
 */
static const struct regmap_access_table it66121_read_table = {
	.yes_ranges = it66121_rw_regs_range,
	.n_yes_ranges = ARRAY_SIZE(it66121_rw_regs_range),
};

static const struct regmap_access_table it66121_write_table = {
	.yes_ranges = it66121_rw_regs_range,
	.n_yes_ranges = ARRAY_SIZE(it66121_rw_regs_range),
	.no_ranges = it66121_ro_regs_range,
	.n_no_ranges = ARRAY_SIZE(it66121_ro_regs_range),
};

static const struct regmap_access_table it66121_volatile_table = {
	.yes_ranges = it66121_rw_regs_range,
	.n_yes_ranges = ARRAY_SIZE(it66121_rw_regs_range),
};

static const struct regmap_range_cfg it66121_regmap_range_cfg[] = {
	{
		.name = "it66121_registers",
		.range_min = IT66121_REG_START,
		.range_max = IT66121_REG_MAX,
		.window_start = IT66121_REG_START,
		.window_len = IT66121_REG_END,
		.selector_reg = 0x0f,
		.selector_mask = 0x0f,
		.selector_shift = 0,
	},
};

static const struct regmap_config it66121_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
	.max_register = IT66121_REG_MAX,
	.ranges = it66121_regmap_range_cfg,
	.num_ranges = ARRAY_SIZE(it66121_regmap_range_cfg),

	.fast_io = false,
	.cache_type = REGCACHE_RBTREE,

	.reg_defaults_raw = it66121_reg_defaults_raw,
	.num_reg_defaults_raw = ARRAY_SIZE(it66121_reg_defaults_raw),

	.volatile_reg = it66121_register_volatile,
	.rd_table = &it66121_read_table,
	.wr_table = &it66121_write_table,
	.volatile_table = &it66121_volatile_table,
};

static ssize_t it66121_resource_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;
	struct bridge_resource *it66121_res;
	struct regmap *regmap;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	it66121_res = &it66121_dev->res;
	DRM_DEV_INFO(dev, "It66121 irq_gpio=%d, irq=%d\n",
		     it66121_res->irq_gpio, it66121_dev->irq);
	regmap = it66121_dev->it66121_regmap;
	DRM_DEV_INFO(dev, "echo 'init' or 'release' it66121_regmap %pK\n",
		     regmap);

	return 0;
}

static ssize_t it66121_resource_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;
	struct regmap *regmap;
	int ret;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);

	if (sysfs_streq(buf, "init")) {
		regmap = devm_regmap_init_i2c(i2c, &it66121_regmap_config);
		if (IS_ERR(regmap)) {
			ret = PTR_ERR(regmap);
			return -ENODEV;
		}
		it66121_dev->it66121_regmap = regmap;
	} else if (sysfs_streq(buf, "release")) {
		regmap = it66121_dev->it66121_regmap;
		regmap_exit(regmap);
		it66121_dev->it66121_regmap = NULL;
	} else {
		DRM_DEV_ERROR(&i2c->dev,
			      "Invalid arguments, echo 'init' or 'release'\n");
	}
	return count;
}

static ssize_t it66121_registers_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	ret = it66121_reg_dump_all(it66121_dev);
	if (!ret) {
		DRM_DEV_ERROR(&i2c->dev, "Failed to dump it66121 registers\n");
		return -EIO;
	}
	return 0;
}

static ssize_t it66121_registers_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int ret;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	unsigned int reg, val;
	u8 str_reg[3], str_val[3];

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);

	DRM_DEV_ERROR(&i2c->dev, "|IT66121 echo| %s, count=%lu", buf, count);
	switch (count) {
	case 5:
		if (sysfs_streq(buf, "init")) {
			DRM_DEV_INFO(&i2c->dev, "init IT66121 register\n");
			it66121_reg_init(it66121_dev);
			it66121_get_status(it66121_dev);
			return count;
		}
	case 6:
		str_reg[0] = buf[3];
		str_reg[1] = buf[4];
		str_reg[2] = '\0';
		DRM_DEV_ERROR(&i2c->dev, "|IT66121 read|str_reg=%s\n", str_reg);

		ret = kstrtouint(str_reg, 16, &reg);
		DRM_DEV_ERROR(&i2c->dev, "|IT66121|read reg[%x]\n", reg);

		regmap_read(it66121_dev->it66121_regmap, reg, &val);
		DRM_DEV_ERROR(&i2c->dev, "|IT66121|Reg[%x]=%x\n", reg, val);
		return count;
	case 11:
		str_reg[0] = buf[3];
		str_reg[1] = buf[4];
		str_reg[2] = '\0';
		str_val[0] = buf[8];
		str_val[1] = buf[9];
		str_val[2] = '\0';
		DRM_DEV_ERROR(&i2c->dev,
			      "|IT66121 writr|str_reg=%s, str_val=%s\n",
			      str_reg, str_val);

		ret = kstrtouint(str_reg, 16, &reg);
		ret = kstrtouint(str_val, 16, &val);

		DRM_DEV_ERROR(&i2c->dev, "|IT66121|write reg[%x], val=%x\n",
			      reg, val);
		regmap_write(it66121_dev->it66121_regmap, reg, val);
		DRM_DEV_ERROR(&i2c->dev, "|IT66121|Reg[%x]=%x\n", reg, val);
		return count;

	default:
		DRM_DEV_ERROR(&i2c->dev, "Error count number %ld\n", count);
		DRM_DEV_INFO(&i2c->dev,
			     "Input such as 'r0x10' or 'w0x10 0x20' \n");
		return -EINVAL;
	}
}

static ssize_t it66121_edid_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int ret, i;
	u8 *pbuf;
	u32 len;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);

	len = EDID_LENGTH;
	pbuf = kzalloc(len, GFP_KERNEL);
	if (IS_ERR(pbuf)) {
		ret = PTR_ERR(pbuf);
		return -ENOMEM;
	}
	ret = it66121_get_edid_block(it66121_dev, pbuf, 0, len);
	DRM_DEV_INFO(to_dev(it66121_dev), "Get EDID[0] %dB, ret %d\n", len,
		     ret);
	for (i = 0; i < len; i++)
		pr_info("buf[%d]=%x", i, pbuf[i]);
	DRM_DEV_INFO(to_dev(it66121_dev), "Dump it66121_dev->fifo_buf\n");
	for (i = 0; i < IT66121_FIFO_MAX_LENGTH; i++)
		pr_info("fifo_buf[%d]=%x", i, it66121_dev->fifo_buf[i]);
	DRM_DEV_INFO(to_dev(it66121_dev), "Dump it66121_dev->edid_buf\n");
	for (i = 0; i < len; i++)
		pr_info("edid_buf[%d]=%x", i, it66121_dev->edid_buf[i]);

	kfree(pbuf);
	return ret;
}

static ssize_t it66121_edid_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int i;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);

	if (sysfs_streq(buf, "edid")) {
		for (i = 0; i < EDID_LENGTH; i++)
			pr_info("edid_buf[%d]=%x", i, it66121_dev->edid_buf[i]);
	} else if (sysfs_streq(buf, "fifo")) {
		for (i = 0; i < IT66121_FIFO_MAX_LENGTH; i++)
			pr_info("fifo_buf[%d]=%x", i, it66121_dev->fifo_buf[i]);
	} else {
		DRM_DEV_ERROR(&i2c->dev, "Invalid, echo 'edid' or 'fifo'\n");
	}

	return count;
}

static ssize_t it66121_irq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	unsigned int sys;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;
	struct regmap *regmap;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	regmap = it66121_dev->it66121_regmap;
	ret = regmap_read(regmap, IT66121_REG_SYS_STATUS, &sys);
	DRM_DEV_INFO(&i2c->dev, "System status  [0E]=%x\n", sys);
	ret = test_bit(SYS_STATUS_INT_ACTIVE_POS, (unsigned long *)&sys);
	DRM_DEV_INFO(&i2c->dev, "System status INT_ACTIVE  [0E]7=%d\n", ret);
	return 0;
}

static ssize_t it66121_irq_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	u32 irq_gpio;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;
	struct bridge_resource *it66121_res;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	it66121_res = &it66121_dev->res;
	irq_gpio = it66121_res->irq_gpio;

	DRM_DEV_INFO(&i2c->dev, "|IT66121 echo| %s, count=%lu\n", buf, count);
	return count;
}

static ssize_t it66121_hpd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	unsigned int int_status;
	bool hpd_status;
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;
	struct regmap *regmap;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	regmap = it66121_dev->it66121_regmap;

	ret = regmap_read(regmap, IT66121_REG_INT_STATUS(INT_STATUS0),
			  &int_status);
	if (ret < 0)
		return -EIO;
	hpd_status = test_bit(INT_HPD_EVENT_POS, (unsigned long *)&int_status);
	if (hpd_status == INT_HPD_EVENT)
		DRM_DEV_INFO(&i2c->dev, "it66121 hpd interrupt on\n");
	else
		DRM_DEV_INFO(&i2c->dev, "it66121 hpd interrupt off\n");
	return 0;
}
static ssize_t it66121_hpd_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);

	if (sysfs_streq(buf, "on")) {
		it66121_int_enable_hpd(it66121_dev);
	} else if (sysfs_streq(buf, "off")) {
		it66121_int_disable_hpd(it66121_dev);
	} else {
		DRM_DEV_ERROR(
			&i2c->dev,
			"Invalid arguments, echo 'on' or 'off' to set hpd int\n");
	}

	return count;
}

static ssize_t it66121_avi_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *i2c;
	struct it66121 *it66121_dev;

	i2c = to_i2c_client(dev);
	it66121_dev = (struct it66121 *)i2c_get_clientdata(i2c);
	DRM_INFO("Modeline " DRM_MODE_FMT "\n",
		 DRM_MODE_ARG(it66121_dev->duplicated_mode));

	return 0;
}

static ssize_t it66121_avi_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	int ret;
	char *recv, *p;
	unsigned int vic;
	struct it66121 *it66121_dev;
	struct drm_display_mode *mode;

	DRM_INFO("Get buf=%s, size=%ld\n", buf, count);
	recv = kstrndup(buf, count, GFP_KERNEL);
	if (IS_ERR(recv))
		return PTR_ERR(recv);
	p = strchr(recv, '\n');
	if (p)
		*p = '\0';
	ret = kstrtouint(recv, 16, &vic);
	DRM_INFO("Duplicate VIC-%d and set avi infoframe\n", vic);
	if (vic == 0 || vic > 100)
		return -EINVAL;

	mode = drm_mode_duplicate(it66121_dev->bridge.dev, &cea_modes[vic]);
	it66121_dev->duplicated_mode = mode;
	it66121_set_avi_infoframe(it66121_dev, mode);
	return count;
}

static DEVICE_ATTR_RW(it66121_resource);
static DEVICE_ATTR_RW(it66121_registers);
static DEVICE_ATTR_RW(it66121_edid);
static DEVICE_ATTR_RW(it66121_irq);
static DEVICE_ATTR_RW(it66121_hpd);
static DEVICE_ATTR_RW(it66121_avi);

/**
 * @section IT66121 driver initialize
 */
static int it66121_get_resources(struct bridge_resource *res,
				 struct it66121 *it66121_dev)
{
	struct loongson_encoder *ls_encoder;

	ls_encoder = res->ls_encoder;
	it66121_dev->encoder = &res->ls_encoder->base;
	it66121_dev->bridge.driver_private = it66121_dev;
	it66121_dev->li2c = &res->ldev->i2c_bus[ls_encoder->encoder_id];

	DRM_DEBUG("Get resource: Encoder [0x%x-%s]: i2c%d-%x irq(%d,%d)\n",
		  res->encoder_obj, res->chip_name, res->i2c_bus_num,
		  res->i2c_dev_addr, res->gpio_placement, res->irq_gpio);
	memcpy(&it66121_dev->res, res, sizeof(it66121_dev->res));

	return 0;
}

static int it66121_register_i2c_device(struct it66121 *it66121_dev)
{
	int ret;
	struct i2c_adapter *i2c_adap;
	struct i2c_client *i2c_cli;
	struct bridge_resource *it66121_res;
	struct i2c_board_info it66121_board_info;

	it66121_res = &it66121_dev->res;
	i2c_adap = it66121_dev->li2c->adapter;
	if (IS_ERR(i2c_adap)) {
		ret = PTR_ERR(i2c_adap);
		DRM_ERROR("Failed to get i2c adapter %d\n",
			  it66121_res->i2c_bus_num);
		return ret;
	}

	memset(&it66121_board_info, 0, sizeof(struct i2c_board_info));
	strncpy(it66121_board_info.type, it66121_res->chip_name, I2C_NAME_SIZE);
	it66121_board_info.dev_name = "it66121";
	it66121_board_info.addr = it66121_res->i2c_dev_addr;
	DRM_INFO("Add encoder PHY: %s@i2c%d-0x%x\n", it66121_board_info.type,
		 i2c_adap->nr, it66121_board_info.addr);
	i2c_cli = i2c_new_device(i2c_adap, &it66121_board_info);
	if (IS_ERR(i2c_cli)) {
		ret = PTR_ERR(i2c_cli);
		DRM_ERROR("Failed to create i2c client for it66121\n");
		return ret;
	}

	i2c_set_clientdata(i2c_cli, it66121_dev);
	it66121_dev->i2c_it66121 = i2c_cli;
	i2c_put_adapter(i2c_adap);
	DRM_DEBUG("Create it66121 i2c-client [%d-00%x]\n",
		  it66121_res->i2c_bus_num, it66121_res->i2c_dev_addr);
	return 0;
}

static int it66121_regmap_init(struct it66121 *it66121_dev)
{
	int ret;
	struct regmap *regmap;

	mutex_init(&it66121_dev->ddc_status.ddc_bus_mutex);
	atomic_set(&it66121_dev->irq_status, 0);

	regmap = devm_regmap_init_i2c(it66121_dev->i2c_it66121,
				      &it66121_regmap_config);
	if (IS_ERR(regmap)) {
		ret = PTR_ERR(regmap);
		return -ret;
	}
	it66121_dev->it66121_regmap = regmap;

	return 0;
}

static int it66121_register_irq(struct it66121 *it66121_dev)
{
	int ret;
	struct bridge_resource *it66121_res;

	it66121_res = &it66121_dev->res;
	if (it66121_res->gpio_placement)
		it66121_res->irq_gpio += IT66121_GPIO_PLACEMENT_OFFSET;

	ret = gpio_is_valid(it66121_res->irq_gpio);
	if (!ret)
		goto error_gpio_valid;
	ret = gpio_request(it66121_res->irq_gpio, "it66121_interrupt");
	if (ret)
		goto error_gpio;
	ret = gpio_direction_input(it66121_res->irq_gpio);
	if (ret)
		goto error_gpio;
	it66121_dev->irq = gpio_to_irq(it66121_res->irq_gpio);

	ret = devm_request_threaded_irq(
		to_dev(it66121_dev), it66121_dev->irq, NULL, it66121_isr_thread,
		IRQF_ONESHOT | IRQF_SHARED | IRQF_TRIGGER_HIGH, "IT66121_INT",
		it66121_dev);
	if (ret)
		goto error_irq;

	DRM_DEV_DEBUG(to_dev(it66121_dev), "IT66121 register irq %d\n",
		      it66121_dev->irq);

	return 0;

error_irq:
	DRM_ERROR("Failed to request irq, ret=%d\n", ret);
error_gpio:
	DRM_ERROR("Failed to config, free gpio, ret=%d\n", ret);
	gpio_free(it66121_res->irq_gpio);
error_gpio_valid:
	DRM_ERROR("Failed to request gpio %d, ret=%d\n", it66121_res->irq_gpio,
		  ret);
	return -ENODEV;
}

static int it66121_unregister_irq(struct it66121 *it66121_dev)
{
	struct bridge_resource *it66121_res;

	it66121_res = &it66121_dev->res;
	if (gpio_is_valid(it66121_res->irq_gpio)) {
		free_irq(gpio_to_irq(it66121_res->irq_gpio), it66121_dev);
		gpio_free(it66121_res->irq_gpio);
	}

	return 0;
}

static int it66121_hw_init(struct it66121 *it66121_dev)
{
	it66121_register_i2c_device(it66121_dev);
	it66121_regmap_init(it66121_dev);
	it66121_register_irq(it66121_dev);

	return 0;
}

static int it66121_debugfs_init(struct it66121 *it66121_dev)
{
	int ret;
	struct device *dev;

	dev = to_dev(it66121_dev);
	ret = device_create_file(dev, &dev_attr_it66121_resource);
	if (ret)
		goto error;
	ret = device_create_file(dev, &dev_attr_it66121_registers);
	if (ret)
		goto error;
	ret = device_create_file(dev, &dev_attr_it66121_edid);
	if (ret)
		goto error;
	ret = device_create_file(dev, &dev_attr_it66121_irq);
	if (ret)
		goto error;
	ret = device_create_file(dev, &dev_attr_it66121_hpd);
	if (ret)
		goto error;
	ret = device_create_file(dev, &dev_attr_it66121_avi);
	if (ret)
		goto error;

	return 0;
error:
	DRM_DEV_ERROR(dev, "Failed to create device file for IT66121\n");
	return -ENXIO;
}

static int it66121_sw_init(struct it66121 *it66121_dev)
{
	int ret;

	it66121_dev->funcs->reg_init(it66121_dev);
	it66121_dev->funcs->power_on(it66121_dev);
	it66121_dev->funcs->set_afe_high(it66121_dev);
	it66121_dev->funcs->set_hdmi_mode(it66121_dev);
	ret = it66121_dev->funcs->debugfs_init(it66121_dev);

	return ret;
}

static int it66121_bridge_bind(struct it66121 *it66121_dev)
{
	int ret;

	it66121_dev->bridge.funcs = &it66121_bridge_funcs;
	drm_bridge_add(&it66121_dev->bridge);
	ret = drm_bridge_attach(it66121_dev->encoder, &it66121_dev->bridge,
				NULL);
	if (ret) {
		DRM_DEV_ERROR(to_dev(it66121_dev),
			      "Failed to attach encoder\n");
		return ret;
	}
	DRM_INFO("IT66121 attach to encoder-%d, type %d\n",
		 it66121_dev->encoder->index,
		 it66121_dev->encoder->encoder_type);

	return 0;
}

static int it66121_int_init(struct it66121 *it66121_dev)
{
	int ret;
	struct regmap *regmap;

	regmap = it66121_dev->it66121_regmap;

	ret = it66121_int_enable_hpd(it66121_dev);
	DRM_DEV_DEBUG(to_dev(it66121_dev),
		      "Enable it66121 HPD interrupt and irq\n");

	return ret;
}

int loongson_it66121_init(struct bridge_resource *res)
{
	int ret;
	struct it66121 *it66121_dev;

	it66121_dev = kzalloc(sizeof(*it66121_dev), GFP_KERNEL);
	if (IS_ERR(it66121_dev)) {
		ret = PTR_ERR(it66121_dev);
		return -ENOMEM;
	}
	it66121_dev->funcs = &it66121_cfg_funcs;

	it66121_get_resources(res, it66121_dev);
	it66121_hw_init(it66121_dev);
	it66121_sw_init(it66121_dev);

	it66121_chip_id_verify(it66121_dev);
	it66121_bridge_bind(it66121_dev);
	it66121_int_init(it66121_dev);
	res->ldev->mode_info[res->mode_info_index].connector = NULL;
	res->ldev->mode_info[res->mode_info_index].mode_config_initialized =
		true;

	return 0;
}

int it66121_remove(struct it66121 *it66121_dev)
{
	it66121_hdcp_enable(it66121_dev);
	it66121_int_enable_all(it66121_dev);
	it66121_ddc_hdcp_reset(it66121_dev);
	it66121_afe_low(it66121_dev);
	it66121_int_disable_all(it66121_dev);
	it66121_unregister_irq(it66121_dev);
	kfree(it66121_dev);
	it66121_dev = NULL;

	return 0;
}

MODULE_AUTHOR("sunhao <sunhao@loongson.cn>");
MODULE_DESCRIPTION("IT66121 HDMI encoder driver");
MODULE_LICENSE("GPL");
