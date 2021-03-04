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

#include <drm/drm_gem_cma_helper.h>

#include "lsdc_drv.h"
#include "lsdc_cursor.h"
#include "lsdc_hardware.h"

/*
 * Hide the cursor off screen. We can't disable the cursor hardware because it
 * takes too long to re-activate and causes momentary corruption
 */
static void loongson_hide_cursor(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_drm_device *ldev = to_loongson_private(dev);
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	unsigned int tmp;
	unsigned int crtc_id = loongson_crtc->crtc_id;

	tmp = ls_reg_read32(ldev, FB_CUR_CFG_REG);
	tmp &= ~0xff;

	if (clone_mode(ldev)) {
		ls_reg_write32(ldev, FB_CUR_CFG_REG, tmp);
	} else {
		if (ldev->cursor_crtc_id != crtc_id)
			return;

		ls_reg_write32(ldev, FB_CUR_CFG_REG,
					crtc_id ? (tmp | 0x10) : tmp);
	}

	ldev->cursor_showed = false;
}

static void loongson_show_cursor(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct loongson_drm_device *ldev = to_loongson_private(dev);
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	unsigned int crtc_id = loongson_crtc->crtc_id;

	unsigned int tmp = 0x00050202;

	if (clone_mode(ldev)) {

		ls_reg_write32(ldev, FB_CUR_CFG_REG, tmp);

		ldev->cursor_crtc_id = 0;
	} else {
		if (ldev->cursor_crtc_id == crtc_id) {

			ls_reg_write32(ldev, FB_CUR_CFG_REG,
				crtc_id ? (tmp | 0x10) : tmp);

			ldev->cursor_crtc_id = crtc_id;
		}
	}

	ldev->cursor_showed = true;
}


int loongson_crtc_cursor_set2(struct drm_crtc *crtc,
			struct drm_file *file_priv,
			uint32_t handle,
			uint32_t width,
			uint32_t height,
			int32_t hot_x,
			int32_t hot_y)
{
	int ret = 0;
	struct drm_gem_object *obj;
	struct drm_device *dev = crtc->dev;
	struct loongson_drm_device *ldev = to_loongson_private(dev);
	struct drm_gem_cma_object *cma;
	struct drm_gem_cma_object *cursor = ldev->cursor;


	if (((width != 32) || (height != 32)) && handle)
		return -EINVAL;

	if (!handle || !file_priv) {
		loongson_hide_cursor(crtc);
		return 0;
	}

	obj = drm_gem_object_lookup(file_priv, handle);
	if (!obj)
		return -ENOENT;

	cma = to_drm_gem_cma_obj(obj);

	memcpy(cursor->vaddr, cma->vaddr, 32*32*4);

	/* Program DC's address of cursor buffer */
	ls_reg_write32(ldev, FB_CUR_ADDR_REG, cursor->paddr);
	ls_reg_write32(ldev, FB_CUR_BACK_REG, 0x00eeeeee);
	ls_reg_write32(ldev, FB_CUR_FORE_REG, 0x00aaaaaa);

	loongson_show_cursor(crtc);
	ret = 0;

	if (ret)
		loongson_hide_cursor(crtc);

	return ret;
}

int loongson_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	unsigned int tmp;
	int xorign = 0, yorign = 0;
	struct loongson_crtc *loongson_crtc = to_loongson_crtc(crtc);
	struct loongson_drm_device *ldev = to_loongson_private(crtc->dev);

	unsigned int crtc_id = loongson_crtc->crtc_id;

	/* upper edge condition */
	yorign = y + crtc->y;
	if (yorign < 0)
		y = 0;

	/* left edge conditon */
	xorign = x + crtc->x;
	if (xorign < 0)
		x = 0;

	/* move from one crtc to another, check which crtc should he shown
	 * the x or y < 0, it means the cursor it out of current review,
	 * && xorign/ yorign > 0, it means the cursor is in the framebuffer
	 * but not in curren review
	 */
	if ((x < 0 && xorign > 0) || (y < 0 && yorign > 0)) {
		/* the cursor is not show, so hide if the (x,y)
		 * is in active crtc
		 */
		if ((ldev->cursor_crtc_id == crtc_id) && !clone_mode(ldev))
			loongson_hide_cursor(crtc);

		return 0;
	}

	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	tmp = x & 0xffff;
	tmp |= y << 16;

	ls_reg_write32(ldev, FB_CUR_LOC_ADDR_REG, tmp);

	if (ldev->cursor_crtc_id != crtc_id && !clone_mode(ldev)) {
		ldev->cursor_crtc_id = crtc_id;
		ldev->cursor_showed = false;
	}

	if (!ldev->cursor_showed)
		loongson_show_cursor(crtc);

	return 0;
}
