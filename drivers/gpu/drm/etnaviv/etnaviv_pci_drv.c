// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015-2018 Etnaviv Project
 */

#include <linux/component.h>
#include <linux/of_platform.h>
#include <linux/thermal.h>
#include <drm/drm_of.h>

#include "etnaviv_cmdbuf.h"
#include "etnaviv_drv.h"
#include "etnaviv_gpu.h"
#include "etnaviv_gem.h"
#include "etnaviv_mmu.h"
#include "etnaviv_perfmon.h"
#include "etnaviv_sched.h"

#ifdef CONFIG_CPU_LOONGSON3
#include "etnaviv_pci_drv.h"
#endif


#ifdef CONFIG_CPU_LOONGSON3

enum vivante_gpu_family {
	GC1000 = 0,
	CHIP_LAST,
};

static const struct pci_device_id etnaviv_pci_devices[] = {
	{0x0014, 0x7a15, PCI_ANY_ID, PCI_ANY_ID, 0, 0, GC1000},
	{0, 0, 0, 0, 0, 0, 0}
};

static int etnaviv_pci_probe(struct pci_dev *pdev,
				 const struct pci_device_id *ent)

{
	return drm_get_pci_dev(pdev, ent, &etnaviv_drm_driver);
}

static void etnaviv_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	drm_put_dev(dev);
}

struct pci_driver etnaviv_pci_driver = {
	.name = "etnaviv",
	.id_table = etnaviv_pci_devices,
	.probe = etnaviv_pci_probe,
	.remove = etnaviv_pci_remove,
};



int etnaviv_load(struct drm_device *dev, unsigned long flags)
{
	int ret, err;
	struct resource *res;
	struct etnaviv_gpu *gpu;
	struct etnaviv_drm_private *priv;

	if (!dev->pdev)
		return 0;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev->dev_private = priv;
	mutex_init(&priv->gem_lock);
	INIT_LIST_HEAD(&priv->gem_list);
	priv->num_gpus = 0;

	gpu = devm_kzalloc(dev->dev, sizeof(*gpu), GFP_KERNEL);
	if (!gpu)
		return -ENOMEM;

	gpu->dev = dev->dev;
	mutex_init(&gpu->lock);
	mutex_init(&gpu->fence_lock);

	/* Map registers: */
	res = &dev->pdev->resource[0];
	gpu->mmio = devm_ioremap_resource(dev->dev, res);
	if (IS_ERR(gpu->mmio))
		return PTR_ERR(gpu->mmio);

#ifdef CONFIG_CPU_LOONGSON3
	dma_set_mask_and_coherent(dev->dev, DMA_BIT_MASK(32));
#endif

	/* Get Interrupt: */
	gpu->irq = dev->pdev->irq;
	if (gpu->irq < 0) {
		dev_err(dev->dev, "failed to get irq: %d\n", gpu->irq);
		return gpu->irq;
	}

	err = devm_request_irq(dev->dev, gpu->irq, irq_handler, 0,
			       dev_name(gpu->dev), gpu);
	if (err) {
		dev_err(dev->dev, "failed to request IRQ%u: %d\n",
				gpu->irq, err);
		return err;
	}

	/* Get Clocks: */
	gpu->clk_reg = NULL;
	gpu->clk_bus = NULL;
	gpu->clk_core = NULL;
	gpu->clk_shader = NULL;
	gpu->base_rate_core = clk_get_rate(gpu->clk_core);
	gpu->base_rate_shader = clk_get_rate(gpu->clk_shader);

	/* TODO: figure out max mapped size */
	dev_set_drvdata(dev->dev, gpu);

	/*
	 * We treat the device as initially suspended.  The runtime PM
	 * autosuspend delay is rather arbitrary: no measurements have
	 * yet been performed to determine an appropriate value.
	 */
	pm_runtime_use_autosuspend(gpu->dev);
	pm_runtime_set_autosuspend_delay(gpu->dev, 200);

	if (IS_ENABLED(CONFIG_DRM_ETNAVIV_THERMAL)) {
		gpu->cooling = thermal_cooling_device_register(
			(char *)dev_name(dev->dev), gpu, &cooling_ops);
		if (IS_ERR(gpu->cooling))
			return PTR_ERR(gpu->cooling);
	}

	gpu->wq = alloc_ordered_workqueue(dev_name(dev->dev), 0);
	if (!gpu->wq) {
		ret = -ENOMEM;
		goto out_thermal;
	}

	ret = etnaviv_sched_init(gpu);
	if (ret)
		goto out_workqueue;

#ifdef CONFIG_PM
	ret = pm_runtime_get_sync(gpu->dev);
#else
	ret = etnaviv_gpu_clk_enable(gpu);
#endif
	if (ret < 0)
		goto out_sched;


	gpu->drm = dev;
	gpu->fence_context = dma_fence_context_alloc(1);
	idr_init(&gpu->fence_idr);
	spin_lock_init(&gpu->fence_spinlock);

	INIT_WORK(&gpu->sync_point_work, sync_point_worker);
	init_waitqueue_head(&gpu->fence_event);

	priv->gpu[priv->num_gpus++] = gpu;

	pm_runtime_mark_last_busy(gpu->dev);
	pm_runtime_put_autosuspend(gpu->dev);

	load_gpu(dev);

	return 0;

out_sched:
	etnaviv_sched_fini(gpu);

out_workqueue:
	destroy_workqueue(gpu->wq);

out_thermal:
	if (IS_ENABLED(CONFIG_DRM_ETNAVIV_THERMAL))
		thermal_cooling_device_unregister(gpu->cooling);

	return ret;
}


void etnaviv_unload(struct drm_device *dev)
{
	struct etnaviv_gpu *gpu = dev_get_drvdata(dev->dev);

	if (!dev->pdev)
		return;

	DBG("%s", dev_name(gpu->dev));

	flush_workqueue(gpu->wq);
	destroy_workqueue(gpu->wq);

	etnaviv_sched_fini(gpu);

#ifdef CONFIG_PM
	pm_runtime_get_sync(gpu->dev);
	pm_runtime_put_sync_suspend(gpu->dev);
#else
	etnaviv_gpu_hw_suspend(gpu);
#endif

	if (gpu->buffer.suballoc)
		etnaviv_cmdbuf_free(&gpu->buffer);

	if (gpu->cmdbuf_suballoc) {
		etnaviv_cmdbuf_suballoc_destroy(gpu->cmdbuf_suballoc);
		gpu->cmdbuf_suballoc = NULL;
	}

	if (gpu->mmu) {
		etnaviv_iommu_destroy(gpu->mmu);
		gpu->mmu = NULL;
	}

	gpu->drm = NULL;
	idr_destroy(&gpu->fence_idr);

	if (IS_ENABLED(CONFIG_DRM_ETNAVIV_THERMAL))
		thermal_cooling_device_unregister(gpu->cooling);

	gpu->cooling = NULL;
	pm_runtime_disable(dev->dev);
}


MODULE_DEVICE_TABLE(pci, etnaviv_pci_devices);

#endif
