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

#include <linux/errno.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <linux/platform_device.h>

#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#include <drm/drm_irq.h>

#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>

#include "lsdc_drv.h"
#include "lsdc_vbios.h"
#include "lsdc_crtc.h"
#include "lsdc_encoder.h"
#include "lsdc_connector.h"
#include "lsdc_irq.h"


static const struct drm_mode_config_funcs loongson_mode_funcs = {
	.fb_create = drm_gem_fb_create,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};


static int lsn_modeset_init(struct loongson_drm_device *pLsDev)
{
	const uint32_t nCrtc = pLsDev->vbios->crtc_num;
	const unsigned int nChan = pLsDev->vbios->connector_num;
	int i;
	int ret;

	/* ls2k1000 user manual say the pix clock can be about 200MHz */
	pLsDev->clock.max_pixel_clock = 200000;

	DRM_INFO("Total %d crtcs.\n", nCrtc);
	DRM_INFO("Total %d connector.\n", nChan);

	pLsDev->num_crtc = nCrtc;

	for (i = 0; i < nCrtc; ++i) {
		struct loongson_connector *pLsCon;
		struct loongson_mode_info *pModeInfo;
		struct drm_encoder *encoder;

		pModeInfo = &pLsDev->mode_info[i];

		lsn_crtc_init(pLsDev, i);

		pModeInfo->crtc = &pLsDev->lcrtc[i];

		encoder = lsn_encoder_init(pLsDev->dev, i);
		if (encoder == NULL) {
			DRM_ERROR("loongson_encoder_init failed.\n");
			return -1;
		}

		pLsCon = lsn_connector_init(pLsDev, i);
		pModeInfo->connector = pLsCon;

		ret = drm_connector_attach_encoder(&pLsCon->base, encoder);
		if (ret)
			DRM_INFO("failed attach connector to encoder.\n");

		pModeInfo->mode_config_initialized = true;
		drm_mode_config_reset(pLsDev->dev);
	}

	return 0;
}


DEFINE_DRM_GEM_CMA_FOPS(fops);


struct drm_driver loongson_drm_driver = {

	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_PRIME |
				DRIVER_HAVE_IRQ | DRIVER_ATOMIC,

	.fops = &fops,

	.dumb_create = drm_gem_cma_dumb_create,
	.gem_free_object_unlocked = drm_gem_cma_free_object,
	.gem_vm_ops = &drm_gem_cma_vm_ops,

	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,

	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_export = drm_gem_prime_export,

	.gem_prime_get_sg_table = drm_gem_cma_prime_get_sg_table,
	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap = drm_gem_cma_prime_vmap,
	.gem_prime_vunmap = drm_gem_cma_prime_vunmap,
	.gem_prime_mmap = drm_gem_cma_prime_mmap,

	.irq_handler = loongson_irq_handler,
	.irq_preinstall = loongson_irq_preinstall,
	.irq_postinstall = loongson_irq_postinstall,
	.irq_uninstall = loongson_irq_uninstall,
	.name = "loongson-drm",
	.desc = "LOONGSON DRM Driver",
	.date = "20190831",
	.major = 1,
	.minor = 0,
	.patchlevel = 1,
};


static int lsn_detect_pci_chip(struct loongson_drm_device *pLsPriv,
			       const struct pci_device_id *pEnt)
{
	/*
	 * Identify chipset, we just use plain 0014:7a06, as
	 * this is never change and we don't want to include
	 * external headers.
	 */
	if (pEnt->vendor == PCI_VENDOR_ID_LOONGSON) {
		if (pEnt->device == PCI_DEVICE_ID_LOONGSON_DC) {
			pLsPriv->chip = LSDC_CHIP_7A1000;
			pLsPriv->board = LS3A4000_7A1000_EVB_BOARD_V1_4;
			DRM_INFO("Loongson 7A1000 detected.\n");
			DRM_INFO("class: 0x%x.\n", pEnt->class);
			DRM_INFO("driver data: 0x%lx.\n", pEnt->driver_data);
		} else {
			pLsPriv->chip = LSDC_CHIP_UNKNOWN;
			DRM_WARN("Unknown loongson chip.\n");
			DRM_INFO("device id: 0x%x.\n", pEnt->device);
			DRM_INFO("class: 0x%x.\n", pEnt->class);
			DRM_INFO("driver data: 0x%lx.\n", pEnt->driver_data);
		}
	}

	return 0;
}


static int lsn_detect_platform_chip(struct loongson_drm_device *pLsPriv,
				const struct platform_device * const pPlatDev)
{
	struct device_node *np;
	const char *name = NULL;

	for_each_compatible_node(np, NULL, "loongson,ls2k") {
		const char *model = NULL;

		if (!of_device_is_available(np))
			continue;

		of_property_read_string(np, "compatible", &name);

		if (!name)
			continue;

		pLsPriv->chip = LSDC_CHIP_2K1000;
		pLsPriv->board = LS2K1000_PC_EVB_V1_2;

		of_property_read_string(np, "model", &model);
		if (!model) {
			pLsPriv->board = LS_PCB_UNKNOWN;
			pLsPriv->chip = LSDC_CHIP_UNKNOWN;
		} else if (!strncmp(model,
				    "loongson,LS2K1000_PC_EVB_V1_2", 29)) {
			pLsPriv->board = LS2K1000_PC_EVB_V1_2;
			pLsPriv->chip = LSDC_CHIP_2K1000;
		} else if (!strncmp(model,
				    "loongson,LS2K1000_PC_EVB_V1_1", 29)) {
			pLsPriv->board = LS2K1000_PC_EVB_V1_1;
			pLsPriv->chip = LSDC_CHIP_2K1000;
		}

		DRM_INFO("Loongson 2k1000 SoC detected.\n");

		DRM_INFO("model name: %s, compatible: %s\n", model, name);
	}

	return 0;
}



static void lsn_cursor_init(struct loongson_drm_device *pLsDev)
{
	/*
	 * Make small buffers to store a hardware cursor
	 * (double buffered icon updates)
	 */
	struct drm_device *ddev = pLsDev->dev;

	pLsDev->cursor = drm_gem_cma_create(ddev, roundup(32*32*4, PAGE_SIZE));
}


static int lsn_mode_config_init(struct loongson_drm_device *pLsDev, int irq)
{
	struct drm_device *ddev = pLsDev->dev;
	int ret;

	ddev->mode_config.funcs = &loongson_mode_funcs;
	ddev->mode_config.min_width = 1;
	ddev->mode_config.min_height = 1;
	ddev->mode_config.preferred_depth = 24;
	ddev->mode_config.prefer_shadow = 1;
	ddev->mode_config.max_width = LSDC_MAX_FB_HEIGHT;
	ddev->mode_config.max_height = LSDC_MAX_FB_HEIGHT;

	ddev->mode_config.cursor_width = 32;
	ddev->mode_config.cursor_height = 32;

	ddev->mode_config.allow_fb_modifiers = true;

	drm_mode_config_init(ddev);

	ret = drm_irq_install(ddev, irq);
	if (ret)
		dev_err(ddev->dev,
			"Fatal error during irq install: %d\n", ret);

	DRM_INFO("irq installed: irq = %d\n", irq);

	ret = lsn_modeset_init(pLsDev);
	if (ret)
		dev_err(ddev->dev,
			"Fatal error during modeset init: %d\n", ret);

	ret = drm_vblank_init(ddev, pLsDev->num_crtc);
	if (ret)
		dev_err(ddev->dev,
			"Fatal error during vblank init: %d\n", ret);

	drm_kms_helper_poll_init(ddev);

	lsn_cursor_init(pLsDev);

	return 0;
}


int lsn_load_pci_driver(struct drm_device *ddev,
			const struct pci_device_id * const ent)
{
	struct loongson_drm_device *ldev;
	struct resource *ls7a_bar;
	int ret;

	ldev = kzalloc(sizeof(struct loongson_drm_device), GFP_KERNEL);
	if (ldev == NULL)
		return -ENOMEM;

	ddev->dev_private = (void *)ldev;
	ldev->dev = ddev;

	lsn_detect_pci_chip(ldev, ent);

	lsn_vbios_init(ldev);

	ldev->rmmio = pci_iomap(ddev->pdev, 0, 0);
	if (ldev->rmmio == NULL) {
		kfree(ldev);
		ddev->dev_private = NULL;
		return -EIO;
	}

	/*
	 * If we don't have IO space at all, use MMIO.
	 * assume the chip has MMIO enabled by default (rev 0x20 and higher).
	 */
	if (pci_resource_flags(ddev->pdev, 2) & IORESOURCE_IO)
		DRM_INFO("has IO space.\n");
	else
		DRM_INFO("has NO IO space, trying MMIO.\n");

	/* BAR 0 contains registers */
	ldev->rmmio_base = pci_resource_start(ddev->pdev, 0);
	ldev->rmmio_size = pci_resource_len(ddev->pdev, 0);

	ls7a_bar = devm_request_mem_region(ddev->dev,
			ldev->rmmio_base, ldev->rmmio_size,
			"loongson_drm_mmio");

	if (ls7a_bar == NULL) {
		DRM_ERROR("Can't reserve mmio registers\n");
		return -ENOMEM;
	}

	DRM_INFO("MMIO: base = 0x%llx, size = 0x%llx\n",
			ldev->rmmio_base, ldev->rmmio_size);

	ret = lsn_mode_config_init(ldev, ddev->pdev->irq);

	return ret;
}


int lsn_platform_driver_init(struct platform_device *pldev,
			     struct drm_device *ddev)
{
	int irq;
	struct loongson_drm_device *ldev;
	struct resource *res;

	ldev = devm_kzalloc(ddev->dev,
			sizeof(struct loongson_drm_device), GFP_KERNEL);
	if (ldev == NULL)
		return -ENOMEM;

	ddev->dev_private = (void *)ldev;
	ldev->dev = ddev;

	lsn_detect_platform_chip(ldev, pldev);

	lsn_vbios_init(ldev);

	res = platform_get_resource(pldev, IORESOURCE_MEM, 0);

	ldev->rmmio_base = res->start;
	ldev->rmmio_size = resource_size(res);

	ldev->rmmio = ioremap(ldev->rmmio_base, ldev->rmmio_size);
	if (ldev->rmmio == NULL)
		return -ENOMEM;

	if (!devm_request_mem_region(ldev->dev->dev, ldev->rmmio_base,
			ldev->rmmio_size, "loongson_drm_mmio")) {
		DRM_ERROR("Can't reserve mmio registers\n");
		return -ENOMEM;
	}

	DRM_INFO("MMIO: base = 0x%llx, MMIO: size = 0x%llx\n",
			ldev->rmmio_base, ldev->rmmio_size);

	irq = platform_get_irq(pldev, 0);

	lsn_mode_config_init(ldev, irq);

	return 0;
}
