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
#include <linux/console.h>

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>

#include "lsdc_drv.h"


static int lsn_platform_probe(struct platform_device *pldev)
{
	int ret;
	struct drm_device *ddev;

	ret = dma_set_coherent_mask(&pldev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pldev->dev, "Failed to set the DMA mask\n");
		return ret;
	}

	/* Allocate and initialize the driver private structure. */
	ddev = drm_dev_alloc(&loongson_drm_driver, &pldev->dev);
	if (IS_ERR(ddev))
		return PTR_ERR(ddev);

	platform_set_drvdata(pldev, ddev);

	lsn_platform_driver_init(pldev, ddev);

	/*
	 * Register the DRM device with the core and the connectors with sysfs.
	 */
	ret = drm_dev_register(ddev, 0);
	if (ret) {
		drm_dev_put(ddev);
		return ret;
	}

	drm_fbdev_generic_setup(ddev, 32);

	return 0;
}


static int lsn_platform_remove(struct platform_device *pdev)
{
	struct drm_device *ddev = platform_get_drvdata(pdev);

	drm_dev_unregister(ddev);
	drm_mode_config_cleanup(ddev);
	drm_dev_put(ddev);
	ddev->dev_private = NULL;
	dev_set_drvdata(ddev->dev, NULL);

	return 0;
}


#ifdef CONFIG_PM
static int lsn_drm_suspend(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return drm_mode_config_helper_suspend(drm_dev);
}

static int lsn_drm_resume(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return drm_mode_config_helper_resume(drm_dev);
}
#endif


static SIMPLE_DEV_PM_OPS(lsn_pm_ops, lsn_drm_suspend, lsn_drm_resume);


static const struct of_device_id loongson_dc_dt_ids[] = {
	{ .compatible = "loongson,display-subsystem", },
	{ .compatible = "loongson,ls-fb", },
	{}
};
MODULE_DEVICE_TABLE(of, loongson_dc_dt_ids);


struct platform_driver loongson_drm_platform_driver = {
	.probe = lsn_platform_probe,
	.remove = lsn_platform_remove,
	.driver = {
		.name = "loongson-drm",
		.pm = &lsn_pm_ops,
		.of_match_table = of_match_ptr(loongson_dc_dt_ids),
	},
};


static int __init lsn_drm_platform_init(void)
{
#if defined(CONFIG_PCI)
	struct pci_dev *pdev = NULL;

	/* Multi video card workaround */
	while ((pdev = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, pdev))) {
		if (pdev->vendor != PCI_VENDOR_ID_LOONGSON)
			return 0;
	}

	DRM_INFO("Discrete graphic card detected, abort registration.\n");
#endif

	if (vgacon_text_force()) {
		DRM_ERROR("VGACON disables loongson kernel modesetting.\n");
		return -EINVAL;
	}

	return platform_driver_register(&loongson_drm_platform_driver);
}
module_init(lsn_drm_platform_init);


static void __exit lsn_drm_platform_exit(void)
{
	return platform_driver_unregister(&loongson_drm_platform_driver);
}
module_exit(lsn_drm_platform_exit);


MODULE_AUTHOR("Sui Jingfeng <suijingfeng@loongson.cn>");
MODULE_DESCRIPTION("DRM platform Driver for loongson 2k1000");
MODULE_LICENSE("GPL v2");
