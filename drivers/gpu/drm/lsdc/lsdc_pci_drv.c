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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/console.h>
#include <linux/string.h>

#include <drm/drm_print.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>

#include "lsdc_drv.h"



int lsdc_modeset;
MODULE_PARM_DESC(modeset, "Enable/disable CMA-based KMS(1 = enabled, 0 = disabled(default))");
module_param_named(modeset, lsdc_modeset, int, 0644);

static const struct pci_device_id lsn_dc_pciidlist[] = {
	{PCI_VENDOR_ID_LOONGSON, PCI_DEVICE_ID_LOONGSON_DC,
	 PCI_ANY_ID, PCI_ANY_ID, 0, 0, LSDC_CHIP_7A1000},
	{0, 0, 0, 0, 0, 0, 0}
};

MODULE_DEVICE_TABLE(pci, lsn_dc_pciidlist);

static int lsn_pci_probe(struct pci_dev *pdev,
			 const struct pci_device_id * const ent)
{
	int ret;
	struct drm_device *drm;

	if (ent) {
		DRM_INFO("%s, vendor: 0x%x, device: 0x%x.\n",
			__func__, ent->vendor, ent->device);

		DRM_INFO("subvendor: 0x%x, subdevice: 0x%x.\n",
			ent->subvendor, ent->subdevice);

		DRM_INFO("class: 0x%x, class_mask: 0x%x.\n",
			ent->class, ent->class_mask);
	}

	DRM_INFO("PCI slot: %u, func: %u.\n",
		PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));

	ret = pcim_enable_device(pdev);
	if (ret) {
		DRM_ERROR("Enable pci devive failed.\n");
		return ret;
	}

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if (ret == 0)
		DRM_INFO("32-bit DMA operations Support.\n");
	else if (ret == -EIO)
		DRM_WARN("Set DMA Mask failed.\n");
	else
		DRM_WARN("32-bit DMA operations is NOT supported.\n");

	pci_set_master(pdev);


	drm = drm_dev_alloc(&loongson_drm_driver, &pdev->dev);
	if (IS_ERR(drm))
		return PTR_ERR(drm);

	drm->pdev = pdev;
	pci_set_drvdata(pdev, drm);

	ret = lsn_load_pci_driver(drm, ent);
	if (ret)
		DRM_ERROR("loading pci devive driver failed.\n");

	ret = drm_dev_register(drm, ent->driver_data);
	if (ret) {
		pci_disable_device(drm->pdev);
		drm_dev_put(drm);
		return ret;
	}

	drm_fbdev_generic_setup(drm, 32);

	return 0;
}


static void lsn_driver_unload(struct drm_device *dev)
{
	struct loongson_drm_device *pLsDev = to_loongson_private(dev);

	kfree(pLsDev);

	DRM_INFO("loongson driver unloaded.\n");
}


static void lsn_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	drm_dev_unregister(dev);
	lsn_driver_unload(dev);
	drm_dev_put(dev);

	pci_set_drvdata(pdev, NULL);

	pci_clear_master(pdev);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}


static int loongson_drm_suspend(struct drm_device *dev, bool suspend)
{
	struct loongson_drm_device *ldev;

	if ((dev == NULL) || (dev->dev_private == NULL))
		return -ENODEV;

	ldev = dev->dev_private;

	drm_kms_helper_poll_disable(dev);
	ldev->state = drm_atomic_helper_suspend(dev);

	if (dev->pdev) {
		pci_save_state(dev->pdev);
		if (suspend) {
			/* Shut down the device */
			pci_disable_device(dev->pdev);
			pci_set_power_state(dev->pdev, PCI_D3hot);
		}
	}

	console_lock();
	drm_fb_helper_set_suspend(ldev->dev->fb_helper, 1);
	console_unlock();

	return 0;
}


static int loongson_drm_resume(struct drm_device *dev, bool resume)
{
	struct loongson_drm_device *ldev = dev->dev_private;

	console_lock();

	if (resume && dev->pdev) {
		pci_set_power_state(dev->pdev, PCI_D0);
		pci_restore_state(dev->pdev);
		if (pci_enable_device(dev->pdev)) {
			console_unlock();
			return -1;
		}
	}

	/* blat the mode back in */
	drm_atomic_helper_resume(dev, ldev->state);

	drm_kms_helper_poll_enable(dev);

	drm_fb_helper_set_suspend(ldev->dev->fb_helper, 0);

	console_unlock();

	return 0;
}

static int loongson_drm_pm_suspend(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return loongson_drm_suspend(drm_dev, true);
}

static int loongson_drm_pm_resume(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	if (drm_dev->switch_power_state == DRM_SWITCH_POWER_OFF)
		return 0;

	return loongson_drm_resume(drm_dev, true);
}

static int loongson_drm_pm_freeze(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return loongson_drm_suspend(drm_dev, false);
}

static int loongson_drm_pm_thaw(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return loongson_drm_resume(drm_dev, false);
}


const struct dev_pm_ops loongson_drm_pm_ops = {
	.suspend = loongson_drm_pm_suspend,
	.resume = loongson_drm_pm_resume,
	.freeze = loongson_drm_pm_freeze,
	.thaw = loongson_drm_pm_thaw,
	.poweroff = loongson_drm_pm_freeze,
	.restore = loongson_drm_pm_resume,
};



static struct pci_driver lsn_dc_pci_driver = {
	.name = "loongson-drm",
	.id_table = lsn_dc_pciidlist,
	.probe = lsn_pci_probe,
	.remove = lsn_pci_remove,
	.driver.pm = &loongson_drm_pm_ops,
};


static int __init lsn_drm_init(void)
{
	struct pci_dev *pdev = NULL;

	if (lsdc_modeset == 0) {
		DRM_INFO("CMA-based KMS for ls7a1000 is disabled.\n");
		return -ENODEV;
	}

	/*
	 * Enable KMS by default, unless explicitly overridden by
	 * either the lsdc.modeset prarameter or by the
	 * vga_text_mode_force boot option.
	 */
	if (vgacon_text_force()) {
		DRM_ERROR("VGACON disables kernel modesetting driver.\n");
		return -EINVAL;
	}

	/* Multi video card workaround */
	while ((pdev = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, pdev))) {
		if (pdev->vendor != PCI_VENDOR_ID_LOONGSON)
			return 0;
	}

	return pci_register_driver(&lsn_dc_pci_driver);
}


static void __exit lsn_drm_exit(void)
{
	pci_unregister_driver(&lsn_dc_pci_driver);
}

module_init(lsn_drm_init);
module_exit(lsn_drm_exit);

MODULE_AUTHOR("Sui Jingfeng <suijingfeng@loongson.cn>");
MODULE_DESCRIPTION("DRM Driver for LOONGSON 7A1000");
MODULE_LICENSE("GPL v2");
