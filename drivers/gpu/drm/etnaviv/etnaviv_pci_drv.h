/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 Etnaviv Project
 */

#ifndef __ETNAVIV_MODULE_H__
#define __ETNAVIV_MODULE_H__

extern struct drm_driver etnaviv_drm_driver;
extern struct pci_driver etnaviv_pci_driver;

int etnaviv_load(struct drm_device *dev, unsigned long flags);
void etnaviv_unload(struct drm_device *dev);

void load_gpu(struct drm_device *dev);

#endif
