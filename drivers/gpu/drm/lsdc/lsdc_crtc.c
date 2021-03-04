// SPDX-License-Identifier: GPL-2.0+
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

#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>

#include <drm/drm_vblank.h>
#include <drm/drm_fourcc.h>

#include "lsdc_drv.h"
#include "lsdc_vbios.h"
#include "lsdc_plane.h"
#include "lsdc_hardware.h"
#include "lsdc_cursor.h"


static void ls_crtc_write32(struct loongson_drm_device * const ldev,
		     unsigned int crtc_id,
		     u32 reg,
		     u32 val)
{
	u32 offset = reg + crtc_id * LSDC_RB_REG_OFFSET;

	ls_reg_write32(ldev, offset, val);
}


static int lsn_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct loongson_crtc *lcrtc = to_loongson_crtc(crtc);
	unsigned int index = drm_crtc_index(crtc);
	unsigned int init_val;
	unsigned int mask;

	DRM_DEBUG("%s: crtc-%u=%u: enable vblank",
				__func__, lcrtc->crtc_id, index);

	init_val = ls_reg_read32(lcrtc->ldev, LSDC_INT_REG);

	if (index == 0) {
		mask = 1 << LSDC_INT_FB0_VSYNC;
		mask = mask << 16;
	} else {
		mask = 1 << LSDC_INT_FB1_VSYNC;
		mask = mask << 16;
	}

	ls_reg_write32(lcrtc->ldev, LSDC_INT_REG, init_val | mask);

	return 0;
}

static void lsn_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct loongson_crtc *lcrtc = to_loongson_crtc(crtc);
	struct loongson_drm_device *ldev = lcrtc->ldev;
	unsigned int index = drm_crtc_index(crtc);

	unsigned int init_val;
	unsigned int mask;

	DRM_DEBUG("%s: crtc-%u: disable vblank\n", __func__, lcrtc->crtc_id);

	init_val = ls_reg_read32(lcrtc->ldev, LSDC_INT_REG);

	if (index == 0) {
		mask = 1 << LSDC_INT_FB0_VSYNC;
		mask = mask << 16;
	} else {
		mask = 1 << LSDC_INT_FB1_VSYNC;
		mask = mask << 16;
	}

	ls_reg_write32(ldev, LSDC_INT_REG, init_val & ~mask);
}


static void lsn_crtc_destroy(struct drm_crtc *crtc)
{
	struct loongson_crtc *pLsCrtc = to_loongson_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(pLsCrtc);
}


/**
 * These provide the minimum set of functions required to handle a CRTC
 * Each driver is responsible for filling out this structure at startup time
 *
 * The drm_crtc_funcs structure is the central CRTC management structure
 * in the DRM. Each CRTC controls one or more connectors
 */
static const struct drm_crtc_funcs loongson_crtc_funcs = {
	.reset = drm_atomic_helper_crtc_reset,
	.cursor_set2 = loongson_crtc_cursor_set2,
	.cursor_move = loongson_crtc_cursor_move,
	.gamma_set = drm_atomic_helper_legacy_gamma_set,
	.destroy = lsn_crtc_destroy,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank = lsn_crtc_enable_vblank,
	.disable_vblank = lsn_crtc_disable_vblank,
};



static enum drm_mode_status lsn_crtc_mode_valid(struct drm_crtc *crtc,
					const struct drm_display_mode *mode)
{
	int id = crtc->index;
	struct loongson_drm_device *ldev = to_loongson_private(crtc->dev);
	struct loongson_vbios_crtc **pCrtc = ldev->crtc_vbios;

	if (mode->hdisplay > pCrtc[id]->crtc_max_width)
		return MODE_BAD;
	if (mode->vdisplay > pCrtc[id]->crtc_max_height)
		return MODE_BAD;

	if (ldev->num_crtc == 1) {
		if (mode->hdisplay % 16)
			return MODE_BAD;
	} else {
		if (mode->hdisplay % 64)
			return MODE_BAD;
	}

	return MODE_OK;
}

/**
 * @lsn_crtc_mode_set_nofb:
 *
 * This callback is used to update the display mode of a CRTC without
 * changing anything of the primary plane configuration. This fits the
 * requirement of atomic and hence is used by the atomic helpers.
 * It is also used by the transitional plane helpers to implement a
 * @mode_set hook in drm_helper_crtc_mode_set().
 *
 * Note that the display pipe is completely off when this function is called.
 * Atomic drivers which need hardware to be running before they program the
 * new display mode (e.g. because they implement runtime PM) should not use
 * this hook.
 *
 * This is because the helper library calls this hook only once per mode change
 * and not every time the display pipeline is suspended using either DPMS or
 * the new "ACTIVE" property. Which means register values set in this callback
 * might get reset when the CRTC is suspended, but not restored. Such drivers
 * should instead move all their CRTC setup into the @atomic_enable callback.
 *
 * This callback is optional.
 */
static void lsn_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct loongson_crtc *lcrtc = to_loongson_crtc(crtc);
	struct loongson_drm_device *pLsDev = lcrtc->ldev;
	unsigned int crtc_id = lcrtc->crtc_id;
	struct drm_display_mode *mode = &crtc->state->adjusted_mode;

	unsigned int hr = mode->hdisplay;
	unsigned int hss = mode->hsync_start;
	unsigned int hse = mode->hsync_end;
	unsigned int hfl = mode->htotal;

	unsigned int vr = mode->vdisplay;
	unsigned int vss = mode->vsync_start;
	unsigned int vse = mode->vsync_end;
	unsigned int vfl = mode->vtotal;

	unsigned int pix_freq = mode->clock;
	unsigned int stride = crtc->primary->state->fb->pitches[0];

	unsigned int stride_aligned = (stride + 255) & ~255;

	DRM_INFO("crtc_id = %d, pix_freq = %dkHz\n", crtc_id, pix_freq);

	DRM_INFO("hr = %d, hss = %d, hse = %d, hfl = %d\n",
			hr, hss, hse, hfl);

	DRM_INFO("vr = %d, vss = %d, vse = %d, vfl = %d\n",
			vr, vss, vse, vfl);

	ls_crtc_write32(pLsDev, crtc_id, FB_DITCFG_DVO_REG, 0);
	ls_crtc_write32(pLsDev, crtc_id, FB_DITTAB_LO_DVO_REG, 0);
	ls_crtc_write32(pLsDev, crtc_id, FB_DITTAB_HI_DVO_REG, 0);
	ls_crtc_write32(pLsDev, crtc_id, FB_PANCFG_DVO_REG, 0x80001311);
	ls_crtc_write32(pLsDev, crtc_id, FB_PANTIM_DVO_REG, 0);

	ls_crtc_write32(pLsDev, crtc_id, FB_HDISPLAY_DVO_REG,
			(mode->crtc_htotal << 16) | mode->crtc_hdisplay);
	ls_crtc_write32(pLsDev, crtc_id, FB_HSYNC_DVO_REG,
			0x40000000 | (mode->crtc_hsync_end << 16) |
			mode->crtc_hsync_start);

	ls_crtc_write32(pLsDev, crtc_id, FB_VDISPLAY_DVO_REG,
			(mode->crtc_vtotal << 16) | mode->crtc_vdisplay);

	ls_crtc_write32(pLsDev, crtc_id, FB_VSYNC_DVO_REG,
			0x40000000 | (mode->crtc_vsync_end << 16) |
			mode->crtc_vsync_start);

	ls_crtc_write32(pLsDev, crtc_id, FB_STRI_DVO_REG, stride_aligned);

	DRM_INFO("Stride (before aligned): %d\n", stride);
	DRM_INFO("Stride (after aligned): %d\n", stride_aligned);


	switch (crtc->primary->state->fb->format->format) {
	case DRM_FORMAT_RGB565:
		DRM_INFO("FORMAT: DRM_FORMAT_RGB565, 16 bit\n");
		lcrtc->cfg_reg |= 0x3;
		break;
	case DRM_FORMAT_RGB888:
		DRM_INFO("FORMAT: DRM_FORMAT_RGB888, 24 bit\n");
		lcrtc->cfg_reg |= 0x4;
		break;
	case DRM_FORMAT_XRGB8888:
		DRM_INFO("FORMAT: DRM_FORMAT_XRGB8888, 32 bit\n");
		lcrtc->cfg_reg |= 0x4;
		break;
	default:
		DRM_INFO("FORMAT: 0x%x\n",
		crtc->primary->state->fb->format->format);
		lcrtc->cfg_reg |= 0x4;
		break;
	}


	ls_crtc_write32(pLsDev, crtc_id, LSDC_FB0_CFG_REG, lcrtc->cfg_reg);

	lsn_config_pll(pLsDev, crtc_id, pix_freq);
}


static void lsn_crtc_atomic_enable(struct drm_crtc *crtc,
				struct drm_crtc_state *old_state)
{
	struct loongson_crtc *lcrtc = to_loongson_crtc(crtc);

	if (lcrtc->cfg_reg & CFG_ENABLE) {
		DRM_INFO("%s: crtc %d already default\n",
			__func__, lcrtc->crtc_id);
		goto vblank_on;
	}

	DRM_INFO("%s: Enable crtc-%d\n", __func__, lcrtc->crtc_id);

	lcrtc->cfg_reg |= CFG_ENABLE;

	ls_crtc_write32(lcrtc->ldev, lcrtc->crtc_id,
		LSDC_FB0_CFG_REG, lcrtc->cfg_reg);


vblank_on:
	drm_crtc_vblank_on(crtc);
}


static void lsn_crtc_atomic_disable(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_state)
{

	struct loongson_crtc *lcrtc = to_loongson_crtc(crtc);

	lcrtc->cfg_reg &= ~CFG_ENABLE;

	ls_crtc_write32(lcrtc->ldev, lcrtc->crtc_id,
			LSDC_FB0_CFG_REG, lcrtc->cfg_reg);

	DRM_INFO("%s: disable crtc-%d\n", __func__, lcrtc->crtc_id);

	spin_lock_irq(&crtc->dev->event_lock);
	if (crtc->state->event) {
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
	}
	spin_unlock_irq(&crtc->dev->event_lock);

	drm_crtc_vblank_off(crtc);
}


/**
 * @lsn_crtc_atomic_flush:
 *
 * should finalize an atomic update of multiple planes on a CRTC in this hook.
 * Depending upon hardware this might include checking that vblank evasion
 * was successful, unblocking updates by setting bits or setting the GO bit
 * to flush out all updates.
 *
 * Simple hardware or hardware with special requirements can commit and
 * flush out all updates for all planes from this hook and forgo all the
 * other commit hooks for plane updates.
 *
 * This hook is called after any plane commit functions are called.
 *
 * Note that the power state of the display pipe when this function is
 * called depends upon the exact helpers and calling sequence the driver
 * has picked. See drm_atomic_helper_commit_planes() for a discussion of
 * the tradeoffs and variants of plane commit helpers.
 *
 * This callback is used by the atomic modeset helpers and by the
 * transitional plane helpers, but it is optional.
 */

static void lsn_crtc_atomic_flush(struct drm_crtc *crtc,
				  struct drm_crtc_state *old_crtc_state)
{
	struct drm_pending_vblank_event *event = crtc->state->event;

	if (event) {
		spin_lock_irq(&crtc->dev->event_lock);
		if (drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, event);
		else
			drm_crtc_send_vblank_event(crtc, event);
		spin_unlock_irq(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}
}


/**
 * These provide the minimum set of functions required to handle a CRTC
 *
 * The drm_crtc_helper_funcs is a helper operations for CRTC
 */
static const struct drm_crtc_helper_funcs loongson_crtc_helper_funcs = {
	.mode_valid = lsn_crtc_mode_valid,
	.mode_set_nofb = lsn_crtc_mode_set_nofb,
	.atomic_enable = lsn_crtc_atomic_enable,
	.atomic_disable = lsn_crtc_atomic_disable,
	.atomic_flush = lsn_crtc_atomic_flush,
};


/**
 * loongosn_crtc_init
 *
 * @ldev: point to the loongson_drm_device structure
 *
 * Init CRTC
 */
int lsn_crtc_init(struct loongson_drm_device *ldev, int i)
{
	int ret;
	struct drm_plane *pPrimaryPlane;
	struct loongson_crtc *pLsCrtc; /* Binding crtc, not actual one */

	pLsCrtc = &ldev->lcrtc[i];
	pLsCrtc->ldev = ldev;
	pLsCrtc->crtc_id = i;
	pLsCrtc->cfg_reg = CFG_RESET;

	DRM_INFO("%s: Initialize CRTC-%d\n", __func__, i);

	pPrimaryPlane = devm_kzalloc(ldev->dev->dev,
				sizeof(struct drm_plane), GFP_KERNEL);
	if (pPrimaryPlane == NULL)
		return -ENOMEM;

	ret = lsn_primary_plane_init(ldev->dev, pPrimaryPlane, i);
	if (ret) {
		DRM_ERROR("failed to initialize plane.\n");
		return ret;
	}

	ret = drm_crtc_init_with_planes(ldev->dev,
			&pLsCrtc->base, pPrimaryPlane,
			NULL, &loongson_crtc_funcs, NULL);

	if (ret) {
		drm_plane_cleanup(pPrimaryPlane);
		return ret;
	}

	pLsCrtc->primary = pPrimaryPlane;

	ret = drm_mode_crtc_set_gamma_size(&pLsCrtc->base, 256);
	if (ret)
		DRM_WARN("set the gamma table size failed\n");

	drm_crtc_helper_add(&pLsCrtc->base, &loongson_crtc_helper_funcs);

	return 0;
}
