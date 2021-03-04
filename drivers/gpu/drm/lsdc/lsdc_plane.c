// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Loongson Corporation
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
 *	Sui Jingfeng <suijingfeng@loongson.cn>
 */

#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include "lsdc_drv.h"
#include "lsdc_hardware.h"
#include "lsdc_plane.h"

static const uint32_t loongson_primary_formats[] = {
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
};


static bool lsn_format_mod_supported(struct drm_plane *plane,
				     uint32_t format,
				     uint64_t modifier)
{
	DRM_DEBUG_KMS("%s, format = %u. modifier=%llu\n",
		__func__, format, modifier);
	return (modifier == DRM_FORMAT_MOD_LINEAR);
}


static void lsn_plane_atomic_update(struct drm_plane *plane,
				   struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_framebuffer *pFb = state->fb;
	struct loongson_drm_device *ldev;

	if (state && state->fb) {
		unsigned int fb_index;
		struct loongson_crtc *lcrtc;
		unsigned int pitch;
		unsigned int stride;
		unsigned int fb_switch;

		dma_addr_t addr;

		addr = drm_fb_cma_get_gem_addr(pFb, state, 0);

		pitch = state->fb->pitches[0];
		lcrtc = to_loongson_crtc(state->crtc);
		ldev = lcrtc->ldev;

		/* CRTC1 cloned from CRTC0 in clone mode */
		if (clone_mode(ldev))
			ldev->lcrtc[1].cfg_reg |= CFG_PANELSWITCH;
		else
			ldev->lcrtc[1].cfg_reg &= ~CFG_PANELSWITCH;

		stride = (pitch + 255) & ~255;

		if (lcrtc->crtc_id == 0)
			ls_reg_write32(ldev, FB_STRI_DVO_REG, stride);
		else
			ls_reg_write32(ldev, FB_STRI_VGA_REG, stride);


		if (lcrtc->crtc_id == 0)
			fb_index = ls_reg_read32(ldev, LSDC_FB0_CFG_REG);
		else
			fb_index = ls_reg_read32(ldev, LSDC_FB1_CFG_REG);


		if (fb_index & CFG_FBNUM) {
			if (lcrtc->crtc_id == 0)
				ls_reg_write32(ldev, FB_ADDR0_DVO_REG, addr);
			else
				ls_reg_write32(ldev, FB_ADDR0_VGA_REG, addr);

			DRM_INFO("%s: crtc id: %u, dma addr: 0x%llx\n",
				__func__, lcrtc->crtc_id, addr);
		} else {

			if (lcrtc->crtc_id == 0)
				ls_reg_write32(ldev, FB_ADDR1_DVO_REG, addr);
			else
				ls_reg_write32(ldev, FB_ADDR1_VGA_REG, addr);

			DRM_INFO("%s: crtc id: %u, dma addr: 0x%llx\n",
				__func__, lcrtc->crtc_id, addr);
		}

		lcrtc->cfg_reg |= CFG_ENABLE;

		fb_switch = lcrtc->cfg_reg | CFG_ENABLE | CFG_FBSWITCH;

		if (lcrtc->crtc_id == 0)
			ls_reg_write32(ldev, LSDC_FB0_CFG_REG, fb_switch);
		else
			ls_reg_write32(ldev, LSDC_FB1_CFG_REG, fb_switch);
	}
}


static int lsn_plane_atomic_check(struct drm_plane *plane,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	u32 src_x, src_y, src_w, src_h;

	if (!fb)
		return 0;

	/* Convert src_ from 16:16 format */
	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;
	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;

	/* Reject scaling */
	if ((src_w != state->crtc_w) || (src_h != state->crtc_h)) {
		DRM_ERROR("Scaling is not supported");
		return -EINVAL;
	}

	DRM_DEBUG("%s: crtc: %dx%d\n", __func__, state->crtc_w, state->crtc_h);

	return 0;
}


/**
 * lsn_plane_prepare_fb:
 *
 * This hook is to prepare a framebuffer for scanout by e.g. pinning
 * it's backing storage or relocating it into a contiguous block of
 * VRAM. Other possible preparatory work includes flushing caches.
 *
 * This function must not block for outstanding rendering, since it is
 * called in the context of the atomic IOCTL even for async commits to
 * be able to return any errors to userspace. Instead the recommended
 * way is to fill out the &drm_plane_state.fence of the passed-in
 * &drm_plane_state. If the driver doesn't support native fences then
 * equivalent functionality should be implemented through private
 * members in the plane structure.
 *
 * Drivers which always have their buffers pinned should use
 * drm_gem_fb_prepare_fb() for this hook.
 *
 * The helpers will call @cleanup_fb with matching arguments for every
 * successful call to this hook.
 *
 * This callback is used by the atomic modeset helpers and by the
 * transitional plane helpers, but it is optional.
 *
 * RETURNS:
 *
 * 0 on success or one of the following negative error codes allowed by
 * the &drm_mode_config_funcs.atomic_commit vfunc. When using helpers
 * this callback is the only one which can fail an atomic commit,
 * everything else must complete successfully.
 */
static int lsn_plane_prepare_fb(struct drm_plane *plane,
			       struct drm_plane_state *state)
{
	DRM_DEBUG_KMS("PLANE: id=%d, plane->base.id=%d, name=%s\n",
		plane->index, plane->base.id, plane->name);

	/*
	 * Take a reference on the new framebuffer - we want to
	 * hold on to it while the hardware is displaying it.
	 */
	if (state->fb) {
		DRM_DEBUG_KMS("%s: fb->base.id=%d",
			__func__, state->fb->base.id);

		drm_mode_object_get(&state->fb->base);
	}

	return drm_gem_fb_prepare_fb(plane, state);
}

static void lsn_plane_cleanup_fb(struct drm_plane *plane,
			  struct drm_plane_state *old_state)
{
	if (old_state->fb) {
		drm_framebuffer_put(old_state->fb);
		DRM_DEBUG_KMS("%s: %s: base.is=%d, old fb id:%d\n",
			__func__, plane->name, plane->base.id,
			old_state->fb->base.id);
	}
}

static const struct drm_plane_helper_funcs loongson_plane_helper_funcs = {
	.prepare_fb = lsn_plane_prepare_fb,
	.cleanup_fb = lsn_plane_cleanup_fb,
	.atomic_check = lsn_plane_atomic_check,
	.atomic_update = lsn_plane_atomic_update,
};


static const struct drm_plane_funcs loongson_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_primary_helper_destroy,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.format_mod_supported = lsn_format_mod_supported,
};


int lsn_primary_plane_init(struct drm_device *dev,
			   struct drm_plane *pPrimary, int i)
{
	int ret;

	DRM_INFO("Initial primary plane %d.\n", i);

	drm_plane_helper_add(pPrimary, &loongson_plane_helper_funcs);

	ret = drm_universal_plane_init(dev, pPrimary, BIT(i),
				       &loongson_plane_funcs,
				       loongson_primary_formats,
				       ARRAY_SIZE(loongson_primary_formats),
				       NULL,
				       DRM_PLANE_TYPE_PRIMARY, NULL);

	return ret;
}
