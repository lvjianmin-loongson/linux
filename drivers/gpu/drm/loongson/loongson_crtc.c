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

#include "loongson_drv.h"
#include "loongson_vbios.h"

DEFINE_SPINLOCK(loongson_crtc_lock);

/**
 * @loongson_crtc_mode_match
 *
 * @hdisplay
 * @vdisplay
 * @ls_crtc
 * @Returns   struct crtc_vbios_timing *
 */
static struct loongson_timing *
loongson_crtc_get_timing(u32 hdisplay, u32 vdisplay,
			 struct loongson_crtc *ls_crtc)
{
	int match_index = 0;
	struct crtc_timing *timing = ls_crtc->timing;
	struct loongson_timing *config = NULL, *table = NULL;

	table = timing->tables;
	if (!table) {
		DRM_INFO("crtc mode table null \n");
		return config;
	}

	while (match_index < timing->num) {
		if (vdisplay == table->vdisplay &&
		    hdisplay == table->hdisplay) {
			config = table;
			break;
		}

		match_index++;
		table++;
	}

	return config;
}

/*
 * loongson_set_start_address
 *
 * @crtc: point to a drm_crtc structure
 * @offset: framebuffer base address
 */
void loongson_set_start_address(struct drm_crtc *crtc, unsigned offset)
{
	struct loongson_device *ldev;
	struct loongson_crtc *loongson_crtc;
	u32 crtc_offset;

	ldev = crtc->dev->dev_private;
	loongson_crtc = to_loongson_crtc(crtc);
	crtc_offset = loongson_crtc->crtc_offset;

	ls_mm_wreg(ldev, LS_FB_ADDR0_REG + crtc_offset, offset);
	ls_mm_wreg(ldev, LS_FB_ADDR1_REG + crtc_offset, offset);
}

/**
 * loongson_crtc_do_set_base
 *
 * @crtc: point to a drm_crtc structure
 * @fb: point to a drm_framebuffer structure
 * @x: x position on screen
 * @y: y position on screen
 * @atomic: int variable
 *
 * Ast is different - we will force move buffers out of VRAM
 */
static int loongson_crtc_do_set_base(struct drm_crtc *crtc,
				     struct drm_framebuffer *fb, int x, int y,
				     int atomic)
{
	struct loongson_device *ldev = crtc->dev->dev_private;
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	struct drm_gem_object *obj;
	struct loongson_framebuffer *loongson_fb;
	struct loongson_bo *bo;
	u32 depth, ret;
	u32 crtc_address;
	u32 width, pitch;
	u32 cfg_reg = 0;
	u64 gpu_addr;
	u32 crtc_offset;

	crtc_offset = loongson_crtc->crtc_offset;
	ldev = crtc->dev->dev_private;
	width = crtc->primary->fb->width;
	depth = crtc->primary->fb->format->cpp[0] << 3;

	/* push the previous fb to system ram */
	if (!atomic && fb) {
		loongson_fb = to_loongson_framebuffer(fb);
		obj = loongson_fb->obj;
		bo = gem_to_loongson_bo(obj);
		ret = loongson_bo_reserve(bo, false);
		if (ret)
			return ret;
		loongson_bo_unpin(bo);
		loongson_bo_unreserve(bo);
	}

	DRM_DEBUG("crtc width = %d,height = %d\n", width,
		  crtc->primary->fb->height);
	DRM_DEBUG("crtc pitches[0]=%d\n", crtc->primary->fb->pitches[0]);
	loongson_fb = to_loongson_framebuffer(crtc->primary->fb);

	obj = loongson_fb->obj;
	bo = gem_to_loongson_bo(obj);
	ret = loongson_bo_reserve(bo, false);
	if (ret)
		return ret;

	ret = loongson_bo_pin(bo, TTM_PL_FLAG_VRAM, &gpu_addr);
	if (ret) {
		loongson_bo_unreserve(bo);
		return ret;
	}

	ldev->fb_vram_base = gpu_addr;
	if (&ldev->lfbdev->lfb == loongson_fb) {
		/* if pushing console in kmap it */
		ret = ttm_bo_kmap(&bo->bo, 0, bo->bo.num_pages, &bo->kmap);
		if (ret)
			DRM_ERROR("failed to kmap fbcon\n");
	}
	loongson_bo_unreserve(bo);

	cfg_reg = LS_FB_CFG_RESET | LS_FB_CFG_ENABLE;
	switch (depth) {
	case 16:
		cfg_reg |= LS_FB_CFG_FORMAT16;
		break;
	case 15:
		cfg_reg |= LS_FB_CFG_FORMAT15;
		break;
	case 12:
		cfg_reg |= LS_FB_CFG_FORMAT12;
		break;
	case 32:
	case 24:
	default:
		cfg_reg |= LS_FB_CFG_FORMAT32;
		break;
	}

	pitch = crtc->primary->fb->pitches[0];
	switch (depth) {
	case 12 ... 16:
		crtc_address = (u32)gpu_addr + y * pitch + ALIGN(x, 64) * 2;
		break;
	case 24 ... 32:
	default:
		crtc_address = (u32)gpu_addr + y * pitch + ALIGN(x, 64) * 4;
		break;
	}

	ls_mm_wreg(ldev, LS_FB_CFG_REG + crtc_offset, cfg_reg);
	ls_mm_wreg(ldev, LS_FB_STRI_REG + crtc_offset, pitch);

	loongson_set_start_address(crtc, (u32)crtc_address);

	return 0;
}

/**
 * config_pll
 *
 * @pll_base: represent a long type
 * @pll_cfg: point to the pix_pll srtucture
 *
 * Config pll apply to 7a
 */
static void config_pll(struct loongson_device *ldev, unsigned long pll_base,
		       struct pix_pll *pll_cfg)
{
	unsigned long val;

	/* clear sel_pll_out0 */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val &= ~(1UL << 8);
	ls_io_wreg(ldev, pll_base + 0x4, val);
	/* set pll_pd */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val |= (1UL << 13);
	ls_io_wreg(ldev, pll_base + 0x4, val);
	/* clear set_pll_param */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val &= ~(1UL << 11);
	ls_io_wreg(ldev, pll_base + 0x4, val);
	/* clear old value & config new value */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val &= ~(0x7fUL << 0);
	val |= (pll_cfg->l1_frefc << 0); /* refc */
	ls_io_wreg(ldev, pll_base + 0x4, val);
	val = ls_io_rreg(ldev, pll_base + 0x0);
	val &= ~(0x7fUL << 0);
	val |= (pll_cfg->l2_div << 0); /* div */
	val &= ~(0x1ffUL << 21);
	val |= (pll_cfg->l1_loopc << 21); /* loopc */
	ls_io_wreg(ldev, pll_base + 0x0, val);
	/* set set_pll_param */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val |= (1UL << 11);
	ls_io_wreg(ldev, pll_base + 0x4, val);
	/* clear pll_pd */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val &= ~(1UL << 13);
	ls_io_wreg(ldev, pll_base + 0x4, val);
	/* wait pll lock */
	while (!(ls_io_rreg(ldev, pll_base + 0x4) & 0x80))
		cpu_relax();
	/* set sel_pll_out0 */
	val = ls_io_rreg(ldev, pll_base + 0x4);
	val |= (1UL << 8);
	ls_io_wreg(ldev, pll_base + 0x4, val);
}

/**
 * cal_freq
 *
 * @pixclock_khz: unsigned int
 * @pll_config: point to the pix_pll structure
 *
 * Calculate frequency
 */
static unsigned int cal_freq(unsigned int pixclock_khz,
			     struct pix_pll *pll_config)
{
	unsigned int pstdiv, loopc, frefc;
	unsigned long a, b, c;
	unsigned long min = 1000;

	for (pstdiv = 1; pstdiv < 64; pstdiv++) {
		a = (unsigned long)pixclock_khz * pstdiv;
		for (frefc = 3; frefc < 6; frefc++) {
			for (loopc = 24; loopc < 161; loopc++) {
				if ((loopc < 12 * frefc) ||
				    (loopc > 32 * frefc))
					continue;

				b = 100000L * loopc / frefc;
				c = (a > b) ? (a - b) : (b - a);
				if (c < min) {
					pll_config->l2_div = pstdiv;
					pll_config->l1_loopc = loopc;
					pll_config->l1_frefc = frefc;
					return 1;
				}
			}
		}
	}

	return 0;
}

/**
 * loongson_crtc_mode_set_base
 *
 * @crtc: point to a drm_crtc structure
 * @old_fb: point to a drm_crtc structure
 *
 * Transfer the function which is loongson_crtc_do_set_base,and used by
 * the legacy CRTC helpers to set a new framebuffer and scanout position
 */
static int loongson_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y,
				       struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_device *ldev = dev->dev_private;
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	struct drm_display_mode mode = crtc->mode;
	struct loongson_timing *crtc_mode;
	struct pix_pll pll_cfg;

	u32 pix_freq;
	u32 hr, hss, hse, hfl;
	u32 vr, vss, vse, vfl;
	u32 reg_val = 0;
	u32 crtc_offset;

	crtc_offset = loongson_crtc->crtc_offset;
	hr = mode.hdisplay;
	hfl = mode.htotal;
	hse = mode.hsync_end;
	hss = mode.hsync_start;
	vr = mode.vdisplay;
	vfl = mode.vtotal;
	vse = mode.vsync_end;
	vss = mode.vsync_start;
	pix_freq = mode.clock;

	if (loongson_crtc->is_vb_timing) {
		crtc_mode = loongson_crtc_get_timing(hr, vr, loongson_crtc);
		if (!crtc_mode) {
			DRM_ERROR("match  vdisp = %d, hdisp %d no support\n", vr, hr);
			return 1;
		}

		hr = crtc_mode->hdisplay;
		hss = crtc_mode->hsync_start;
		hse = crtc_mode->hsync_start + crtc_mode->hsync_width;
		hfl = crtc_mode->htotal;
		vr = crtc_mode->vdisplay;
		vss = crtc_mode->vsync_start;
		vse = crtc_mode->vsync_start + crtc_mode->vsync_width;
		vfl = crtc_mode->vtotal;
		pix_freq = crtc_mode->clock;
	}

	cal_freq(pix_freq, &pll_cfg);

	config_pll(ldev, LS_PIX_PLL + crtc_offset, &pll_cfg);
	loongson_crtc_do_set_base(crtc, old_fb, x, y, 0);

	ls_mm_wreg(ldev, LS_FB_DITCFG_REG + crtc_offset, 0);
	ls_mm_wreg(ldev, LS_FB_DITTAB_LO_REG + crtc_offset, 0);
	ls_mm_wreg(ldev, LS_FB_DITTAB_HI_REG + crtc_offset, 0);

	reg_val = LS_FB_PANCFG_BASE | LS_FB_PANCFG_DE | LS_FB_PANCFG_CLKEN |
		  LS_FB_PANCFG_CLKPOL;
	ls_mm_wreg(ldev, LS_FB_PANCFG_REG + crtc_offset, reg_val);
	ls_mm_wreg(ldev, LS_FB_PANTIM_REG + crtc_offset, 0);

	ls_mm_wreg(ldev, LS_FB_HDISPLAY_REG + crtc_offset, (hfl << 16) | hr);
	ls_mm_wreg(ldev, LS_FB_HSYNC_REG + crtc_offset,
		   LS_FB_HSYNC_POLSE | (hse << 16) | hss);

	ls_mm_wreg(ldev, LS_FB_VDISPLAY_REG + crtc_offset, (vfl << 16) | vr);
	ls_mm_wreg(ldev, LS_FB_VSYNC_REG + crtc_offset,
		   LS_FB_VSYNC_POLSE | (vse << 16) | vss);

	return 0;
}

/**
 * loongson_crtc_mode_set
 *
 * @crtc: point to the drm_crtc structure
 * @mode: represent a display mode
 * @adjusted_mode: point to the drm_display_mode structure
 * @old_fb: point to the drm_framebuffer structure
 *
 * Used by the legacy CRTC helpers to set a new mode
 */
static int loongson_crtc_mode_set(struct drm_crtc *crtc,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode, int x,
				  int y, struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_device *ldev = dev->dev_private;
	struct loongson_crtc *ls_crtc = to_loongson_crtc(crtc);
	struct loongson_timing *crtc_mode;
	struct pix_pll pll_cfg;
	u32 hr, hss, hse, hfl;
	u32 vr, vss, vse, vfl;
	u32 pix_freq;
	u32 depth;
	u32 crtc_id;
	u32 crtc_offset;
	u32 reg_val = 0;
	u32 ret;

	crtc_offset = ls_crtc->crtc_offset;
	crtc_id = ls_crtc->crtc_id;
	depth = crtc->primary->fb->format->cpp[0] << 3;
	pix_freq = mode->clock;

	hr = mode->hdisplay;
	hss = mode->hsync_start;
	hse = mode->hsync_end;
	hfl = mode->htotal;
	vr = mode->vdisplay;
	vss = mode->vsync_start;
	vse = mode->vsync_end;
	vfl = mode->vtotal;

	if (ls_crtc->is_vb_timing) {
		crtc_mode = loongson_crtc_get_timing(hr, vr, ls_crtc);
		if (!crtc_mode) {
			DRM_ERROR("match  vdisp = %d, hdisp %d no support\n",
				  vr, hr);
			return 1;
		}

		hr = crtc_mode->hdisplay;
		hss = crtc_mode->hsync_start;
		hse = crtc_mode->hsync_start + crtc_mode->hsync_width;
		hfl = crtc_mode->htotal;
		vr = crtc_mode->vdisplay;
		vss = crtc_mode->vsync_start;
		vse = crtc_mode->vsync_start + crtc_mode->vsync_width;
		vfl = crtc_mode->vtotal;
		pix_freq = crtc_mode->clock;
	}

	DRM_DEBUG_KMS("id = %d, depth = %d, pix_freq = %d, x = %d, y = %d\n",
		      crtc_id, depth, pix_freq, x, y);
	DRM_DEBUG_KMS("hr = %d, hss = %d, hse = %d, hfl = %d\n", hr, hss, hse,
		      hfl);
	DRM_DEBUG_KMS("vr = %d, vss = %d, vse = %d, vfl = %d\n", vr, vss, vse,
		      vfl);

	DRM_DEBUG_KMS("fb width = %d, height = %d\n", crtc->primary->fb->width,
		      crtc->primary->fb->height);

	ls_crtc->width = hr;
	ls_crtc->height = vr;
	ret = cal_freq(pix_freq, &pll_cfg);

	if (ret)
		config_pll(ldev, LS_PIX_PLL + crtc_offset, &pll_cfg);

	loongson_crtc_do_set_base(crtc, old_fb, x, y, 0);

	/* these 4 lines cause out of range, because
	 * the hfl hss vfl vss are different with PMON vgamode cfg.
	 * So the refresh freq in kernel and refresh freq in PMON are different.
	 */
	ls_mm_wreg(ldev, LS_FB_DITCFG_REG + crtc_offset, 0);
	ls_mm_wreg(ldev, LS_FB_DITTAB_LO_REG + crtc_offset, 0);
	ls_mm_wreg(ldev, LS_FB_DITTAB_HI_REG + crtc_offset, 0);

	reg_val = LS_FB_PANCFG_BASE | LS_FB_PANCFG_DE | LS_FB_PANCFG_CLKEN |
		  LS_FB_PANCFG_CLKPOL;

	ls_mm_wreg(ldev, LS_FB_PANCFG_REG + crtc_offset, reg_val);
	ls_mm_wreg(ldev, LS_FB_PANTIM_REG + crtc_offset, 0);

	ls_mm_wreg(ldev, LS_FB_HDISPLAY_REG + crtc_offset, (hfl << 16) | hr);
	ls_mm_wreg(ldev, LS_FB_HSYNC_REG + crtc_offset,
		   LS_FB_HSYNC_POLSE | (hse << 16) | hss);

	ls_mm_wreg(ldev, LS_FB_VDISPLAY_REG + crtc_offset, (vfl << 16) | vr);
	ls_mm_wreg(ldev, LS_FB_VSYNC_REG + crtc_offset,
		   LS_FB_VSYNC_POLSE | (vse << 16) | vss);

	if (ls_crtc->cursor.bo)
		loongson_crtc_cursor_move(crtc, ls_crtc->cursor.x, ls_crtc->cursor.y);

	return 0;
}

/**
 * loongson_crtc_dpms
 *
 * @crtc: point to the drm_crtc structure
 * @mode: represent mode
 *
 * According to mode,represent the power levels on the CRTC
 */
static void loongson_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_device *ldev = dev->dev_private;
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	u32 val;
	u32 crtc_offset;

	crtc_offset = loongson_crtc->crtc_offset;

	if (ldev->inited == false) {
		return;
	}

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		val = ls_mm_rreg(ldev, LS_FB_CFG_REG + crtc_offset);
		val |= LS_FB_CFG_ENABLE;
		ls_mm_wreg(ldev, LS_FB_CFG_REG + crtc_offset, val);

		val = ls_mm_rreg(ldev, LS_FB_PANCFG_REG + crtc_offset);
		val |= LS_FB_PANCFG_DE | LS_FB_PANCFG_CLKEN;
		ls_mm_wreg(ldev, LS_FB_PANCFG_REG + crtc_offset, val);

		val = ls_mm_rreg(ldev, LS_FB_HSYNC_REG + crtc_offset);
		val |= LS_FB_HSYNC_POLSE;
		ls_mm_wreg(ldev, LS_FB_HSYNC_REG + crtc_offset, val);
		val = ls_mm_rreg(ldev, LS_FB_VSYNC_REG + crtc_offset);
		val |= LS_FB_VSYNC_POLSE;
		ls_mm_wreg(ldev, LS_FB_VSYNC_REG + crtc_offset, val);
		drm_crtc_vblank_on(crtc);
		loongson_crtc->enabled = true;
		break;
	case DRM_MODE_DPMS_OFF:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		drm_crtc_vblank_off(crtc);
		val = ls_mm_rreg(ldev, LS_FB_PANCFG_REG + crtc_offset);
		val &= ~LS_FB_PANCFG_DE;
		val &= ~LS_FB_PANCFG_CLKEN;
		ls_mm_wreg(ldev, LS_FB_PANCFG_REG + crtc_offset, val);

		val = ls_mm_rreg(ldev, LS_FB_CFG_REG + crtc_offset);
		val &= ~LS_FB_CFG_ENABLE;
		ls_mm_wreg(ldev, LS_FB_CFG_REG + crtc_offset, val);

		val = ls_mm_rreg(ldev, LS_FB_HSYNC_REG + crtc_offset);
		val &= ~LS_FB_HSYNC_POLSE;
		ls_mm_wreg(ldev, LS_FB_HSYNC_REG + crtc_offset, val);
		val = ls_mm_rreg(ldev, LS_FB_VSYNC_REG + crtc_offset);
		val &= ~LS_FB_VSYNC_POLSE;
		ls_mm_wreg(ldev, LS_FB_VSYNC_REG + crtc_offset, val);
		loongson_crtc->enabled = false;
		break;
	}
}

/**
 * loongson_crtc_prepare
 *
 * @crtc: point to a drm_crtc structure
 *
 * This is called before a mode is programmed. A typical use might be to
 * enable DPMS during the programming to avoid seeing intermediate stages
 */
static void loongson_crtc_prepare(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_crtc *crtci;
	/*
	 * The hardware wedges sometimes if you reconfigure one CRTC
	 * whilst another is running
	 */

	DRM_DEBUG("loongson_crtc_prepare\n");
	list_for_each_entry (crtci, &dev->mode_config.crtc_list, head)
		loongson_crtc_dpms(crtci, DRM_MODE_DPMS_OFF);
}

/**
 * loongson_crtc_commit
 *
 * @crtc: point to the drm_crtc structure
 *
 * Commit the new mode on the CRTC after a modeset.This is called after
 * a mode is programmed. It should reverse anything done by the prepare function
 */
static void loongson_crtc_commit(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_crtc *crtci;

	DRM_DEBUG("loongson_crtc_commit\n");
	list_for_each_entry (crtci, &dev->mode_config.crtc_list, head) {
		if (crtci->enabled)
			loongson_crtc_dpms(crtci, DRM_MODE_DPMS_ON);
	}
}

/**
 * loongson_crtc_destroy
 *
 * @crtc: pointer to a drm_crtc struct
 *
 * Destory the CRTC when not needed anymore,and transfer the drm_crtc_cleanup
 * function,the function drm_crtc_cleanup() cleans up @crtc and removes it
 * from the DRM mode setting core.Note that the function drm_crtc_cleanup()
 * does not free the structure itself.
 */
static void loongson_crtc_destroy(struct drm_crtc *crtc)
{
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(loongson_crtc);
}

/**
 * loongosn_crtc_disable
 *
 * @crtc: DRM CRTC
 *
 * Used to shut down CRTC
 */
static void loongson_crtc_disable(struct drm_crtc *crtc)
{
	int ret;
	DRM_DEBUG_KMS("\n");

	loongson_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);
	if (crtc->primary->fb) {
		struct loongson_framebuffer *mga_fb =
			to_loongson_framebuffer(crtc->primary->fb);
		struct drm_gem_object *obj = mga_fb->obj;
		struct loongson_bo *bo = gem_to_loongson_bo(obj);
		ret = loongson_bo_reserve(bo, false);
		if (ret)
			return;
		loongson_bo_unpin(bo);
		loongson_bo_unreserve(bo);
	}
	crtc->primary->fb = NULL;
}

int loongson_crtc_page_flip(struct drm_crtc *crtc, struct drm_framebuffer *fb,
			    struct drm_pending_vblank_event *event,
			    uint32_t page_flip_flags,
			    struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_device *ldev = dev->dev_private;
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	struct loongson_framebuffer *old_fb;
	struct loongson_framebuffer *new_fb;
	struct drm_gem_object *obj;
	struct loongson_flip_work *work;
	struct loongson_bo *new_bo;
	u32 ret;
	u64 base;

	work = kzalloc(sizeof(*work), GFP_KERNEL);
	if (work == NULL)
		return -ENOMEM;

	INIT_DELAYED_WORK(&work->flip_work, loongson_flip_work_func);

	old_fb = to_loongson_framebuffer(crtc->primary->fb);
	obj = old_fb->obj;
	work->old_bo = gem_to_loongson_bo(obj);

	work->event = event;
	work->ldev = ldev;
	work->crtc_id = loongson_crtc->crtc_id;

	new_fb = to_loongson_framebuffer(fb);
	obj = new_fb->obj;
	new_bo = gem_to_loongson_bo(obj);

	/* pinthenewbuffer */
	ret = loongson_bo_reserve(new_bo, false);
	if (unlikely(ret != 0)) {
		DRM_ERROR("failed to reserve new bo buffer before flip\n");
		goto cleanup;
	}

	ret = loongson_bo_pin(new_bo, TTM_PL_FLAG_VRAM, &base);
	if (unlikely(ret != 0)) {
		ret = -EINVAL;
		DRM_ERROR("failed to pin new bo buffer before flip\n");
		goto unreserve;
	}

	loongson_bo_unreserve(new_bo);
	work->base = base;

	ret = drm_crtc_vblank_get(crtc);
	if (ret)
		goto cleanup;

	loongson_crtc->pflip_works = work;
	crtc->primary->fb = fb;

	loongson_flip_work_func(&work->flip_work.work);

	return 0;

unreserve:
	loongson_bo_unreserve(new_bo);

cleanup:
	loongson_bo_unref(&work->old_bo);
	kfree(work);

	return 0;
}

/**
 * These provide the minimum set of functions required to handle a CRTC
 * Each driver is responsible for filling out this structure at startup time
 *
 * The drm_crtc_funcs structure is the central CRTC management structure
 * in the DRM. Each CRTC controls one or more connectors
 */
static const struct drm_crtc_funcs loongson_crtc_funcs = {
	.cursor_set2 = loongson_crtc_cursor_set2,
	.cursor_move = loongson_crtc_cursor_move,
	.set_config = drm_crtc_helper_set_config,
	.destroy = loongson_crtc_destroy,
	.page_flip = loongson_crtc_page_flip,
};

static enum drm_mode_status
loongson_crtc_mode_valid(struct drm_crtc *crtc,
			 const struct drm_display_mode *mode)
{
	if (mode->hdisplay % 64)
		return MODE_BAD;

	return MODE_OK;
}
/**
 * These provide the minimum set of functions required to handle a CRTC
 *
 * The drm_crtc_helper_funcs is a helper operations for CRTC
 */
static const struct drm_crtc_helper_funcs loongson_helper_funcs = {
	.disable = loongson_crtc_disable,
	.dpms = loongson_crtc_dpms,
	.mode_set = loongson_crtc_mode_set,
	.mode_set_base = loongson_crtc_mode_set_base,
	.prepare = loongson_crtc_prepare,
	.commit = loongson_crtc_commit,
	.mode_valid = loongson_crtc_mode_valid,
};

/**
 * loongosn_crtc_init
 *
 * @ldev: point to the loongson_device structure
 *
 * Init CRTC
 */
struct loongson_crtc *loongson_crtc_init(struct loongson_device *ldev,
					 int index)
{
	struct loongson_crtc *ls_crtc;

	ls_crtc = kzalloc(sizeof(struct loongson_crtc) +
				  (1 * sizeof(struct drm_connector *)),
			  GFP_KERNEL);

	if (ls_crtc == NULL)
		return NULL;

	ls_crtc->ldev = ldev;
	ls_crtc->crtc_offset = index * REG_OFFSET;
	ls_crtc->crtc_id = get_crtc_id(ldev, index);
	ls_crtc->max_freq = get_crtc_max_freq(ldev, index);
	ls_crtc->max_width = get_crtc_max_width(ldev, index);
	ls_crtc->max_height = get_crtc_max_height(ldev, index);
	ls_crtc->encoder_id = get_crtc_encoder_id(ldev, index);
	ls_crtc->is_vb_timing = get_crtc_is_vb_timing(ldev, index);

	if (ls_crtc->is_vb_timing)
		ls_crtc->timing = get_crtc_timing(ldev, index);

	drm_crtc_init(ldev->dev, &ls_crtc->base, &loongson_crtc_funcs);

	drm_crtc_helper_add(&ls_crtc->base, &loongson_helper_funcs);

	return ls_crtc;
}
