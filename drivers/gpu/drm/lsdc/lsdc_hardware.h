/* SPDX-License-Identifier: GPL-2.0+ */
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

#ifndef __LSDC_HARDWARE_H__
#define __LSDC_HARDWARE_H__

#include "lsdc_drv.h"


struct pix_pll {
	unsigned int l2_div;
	unsigned int l1_loopc;
	unsigned int l1_frefc;
};

u32 ls_reg_read32(struct loongson_drm_device * const ldev,
		  u32 offset);

void ls_reg_write32(struct loongson_drm_device * const ldev,
		    u32 offset, u32 val);

void lsn_config_pll(struct loongson_drm_device * const pLsDev,
		    int id, unsigned int pix_freq);


#define LO_OFF	0
#define HI_OFF	8

#define CUR_WIDTH_SIZE		32
#define CUR_HEIGHT_SIZE		32

#define CURIOSET_CORLOR		0x4607
#define CURIOSET_POSITION	0x4608
#define CURIOLOAD_ARGB		0x4609
#define CURIOLOAD_IMAGE		0x460A
#define CURIOHIDE_SHOW		0x460B
#define FBEDID_GET		0X860C


#define CFG_FMT			GENMASK(2, 0)
#define CFG_FBSWITCH		BIT(7)
#define CFG_ENABLE		BIT(8)
#define CFG_PANELSWITCH		BIT(9)
#define CFG_FBNUM_BIT		11
#define CFG_FBNUM		BIT(11)
#define CFG_GAMMAR		BIT(12)
#define CFG_RESET		BIT(20)

#define LSDC_FB0_CFG_REG        0x1240
#define LSDC_FB1_CFG_REG        0x1250
#define LSDC_RB_REG_OFFSET      0x10


#define FB_ADDR0_DVO_REG	0x1260
#define FB_ADDR0_VGA_REG	0x1270

#define FB_ADDR1_DVO_REG	0x1580
#define FB_ADDR1_VGA_REG	0x1590

#define FB_STRI_DVO_REG		0x1280
#define FB_STRI_VGA_REG		0x1290

#define FB_DITCFG_DVO_REG	0x1360
#define FB_DITTAB_LO_DVO_REG	0x1380
#define FB_DITTAB_HI_DVO_REG	0x13a0
#define FB_PANCFG_DVO_REG	0x13c0
#define FB_PANTIM_DVO_REG	0x13e0
#define FB_HDISPLAY_DVO_REG	0x1400
#define FB_HSYNC_DVO_REG	0x1420
#define FB_VDISPLAY_DVO_REG	0x1480
#define FB_VSYNC_DVO_REG	0x14a0
#define FB_GAMINDEX_DVO_REG	0x14e0
#define FB_GAMDATA_DVO_REG	0x1500

#define FB_CUR_CFG_REG		0x1520
#define FB_CUR_ADDR_REG		0x1530
#define FB_CUR_LOC_ADDR_REG	0x1540
#define FB_CUR_BACK_REG		0x1550
#define FB_CUR_FORE_REG		0x1560

#define LSDC_INT_REG		0x1570


#define LSDC_INT_FB1_VSYNC             0
#define LSDC_INT_FB1_HSYNC             1
#define LSDC_INT_FB0_VSYNC             2
#define LSDC_INT_FB0_HSYNC             3
#define LSDC_INT_CURSOR_END            4
#define LSDC_INT_FB1_END               5
#define LSDC_INT_FB0_END               6

/* Internal Data Buffer Underflow */
#define LSDC_INT_DB1_UF                7
#define LSDC_INT_DB0_UF                8

/* Internal Data Buffer Fatal Underflow */
#define LSDC_INT_DB1_FUF               9
#define LSDC_INT_DB0_FUF               10

#endif
