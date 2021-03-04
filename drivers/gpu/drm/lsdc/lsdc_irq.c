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
#include <drm/drm_vblank.h>
#include <drm/drm_print.h>

#include "lsdc_drv.h"
#include "lsdc_irq.h"
#include "lsdc_hardware.h"

irqreturn_t loongson_irq_handler(int irq, void *arg)
{
	unsigned int val;
	struct drm_device *dev = (struct drm_device *) arg;
	struct loongson_drm_device *ldev = to_loongson_private(dev);

	val = ls_reg_read32(ldev, LSDC_INT_REG);

	if ((val & 0x03ff) == 0)
		return IRQ_NONE;

	/* disable interrupt */
	ls_reg_write32(ldev, LSDC_INT_REG, 0x0000);


	if (val & 0x80)
		DRM_INFO("VGA buffer underflow\n");

	if (val & 0x100)
		DRM_INFO("DVO buffer underflow\n");


	if (val & 0x200)
		DRM_INFO("Fatal VGA buffer underflow\n");

	if (val & 0x400)
		DRM_INFO("Fatal DVO buffer underflow\n");


	if (val & (1u << LSDC_INT_FB0_VSYNC))
		drm_crtc_handle_vblank(&ldev->lcrtc[0].base);

	if (val & (1u << LSDC_INT_FB1_VSYNC))
		drm_crtc_handle_vblank(&ldev->lcrtc[1].base);

	ls_reg_write32(ldev, LSDC_INT_REG, val & (0xffff << 16));

	return IRQ_HANDLED;
}

void loongson_irq_preinstall(struct drm_device *dev)
{
	struct loongson_drm_device *ldev = to_loongson_private(dev);

	/* disable interrupt */
	ls_reg_write32(ldev, LSDC_INT_REG, 0x0000);
}

int loongson_irq_postinstall(struct drm_device *dev)
{
	struct loongson_drm_device *ldev = to_loongson_private(dev);

	return 0;
}

void loongson_irq_uninstall(struct drm_device *dev)
{
	struct loongson_drm_device *ldev = to_loongson_private(dev);

	ls_reg_write32(ldev, LSDC_INT_REG, 0x0000);
}
