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

#include <drm/drm_print.h>

#include "lsdc_drv.h"
#include "lsdc_hardware.h"


#define LSDC_PLL_REF_CLK_MHZ            100
#define PCLK_PRECISION_INDICATOR        10000


/* Offsets relative to LSDC_XX1000_CFG_REG_BASE (XX=7A, 2K) */
#define LSDC_PIX_PLL_0                  0x04b0
#define LSDC_PIX_PLL_1                  0x04c0

/* LS2K1000 */
#define LSDC_2K1000_REG_BASE            0x1f000000
#define LSDC_2K1000_CFG_REG_BASE        (LSDC_2K1000_REG_BASE + 0x00e10000)

/* LS7A1000 */
#define LSDC_7A1000_REG_BASE            0x10000000
#define LSDC_7A1000_CFG_REG_BASE        (LSDC_7A1000_REG_BASE + 0x00010000)



DEFINE_SPINLOCK(loongson_reglock);

u32 ls_reg_read32(struct loongson_drm_device * const ldev, u32 offset)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&loongson_reglock, flags);
	val = readl(ldev->rmmio + offset);
	spin_unlock_irqrestore(&loongson_reglock, flags);

	return val;
}


void ls_reg_write32(struct loongson_drm_device * const ldev,
			u32 offset, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&loongson_reglock, flags);
	writel(val, ldev->rmmio + offset);
	spin_unlock_irqrestore(&loongson_reglock, flags);
}


static void ls7a_config_pll(void *pll_base, struct pix_pll *pll_cfg)
{
	u32 val;


	/* clear sel_pll_out0 */
	val = readl(pll_base + 0x4);
	val &= ~(1UL << 8);
	writel(val, pll_base + 0x4);

	/* set pll_pd */
	val = readl(pll_base + 0x4);
	val |= (1UL << 13);
	writel(val, pll_base + 0x4);

	/* clear set_pll_param */
	val = readl(pll_base + 0x4);
	val &= ~(1UL << 11);
	writel(val, pll_base + 0x4);

	/* clear old value & config new value */
	val = readl(pll_base + 0x4);
	val &= ~(0x7fUL << 0);
	val |= (pll_cfg->l1_frefc << 0); /* refc */
	writel(val, pll_base + 0x4);

	val = readl(pll_base + 0x0);
	val &= ~(0x7fUL << 0);
	val |= (pll_cfg->l2_div << 0);   /* div */
	val &= ~(0x1ffUL << 21);
	val |= (pll_cfg->l1_loopc << 21);/* loopc */
	writel(val, pll_base + 0x0);

	/* set set_pll_param */
	val = readl(pll_base + 0x4);
	val |= (1UL << 11);
	writel(val, pll_base + 0x4);

	/* clear pll_pd */
	val = readl(pll_base + 0x4);
	val &= ~(1UL << 13);
	writel(val, pll_base + 0x4);

	/* wait pll lock */
	while (!(readl(pll_base + 0x4) & 0x80))
		cpu_relax();
	/* set sel_pll_out0 */
	val = readl(pll_base + 0x4);
	val |= (1UL << 8);
	writel(val, pll_base + 0x4);
}


static unsigned int ls7a_cal_freq(unsigned int pixclock,
				  struct pix_pll *pll_config)
{
	int i, j, loopc_offset;
	unsigned int refc_set[] = {4, 5, 3};
	/* in 1/PCLK_PRECISION_INDICATOR */
	unsigned int prec_set[] = {1, 5, 10, 50, 100};
	unsigned int pstdiv, loopc, refc;
	unsigned int precision_req, precision;
	unsigned int loopc_min, loopc_max, loopc_mid;
	unsigned long long real_dvo, req_dvo;
	unsigned int base;

	/* try precision from high to low */
	for (j = 0; j < sizeof(prec_set)/sizeof(int); j++) {
		precision_req = prec_set[j];

		/* try each refc */
		for (i = 0; i < sizeof(refc_set)/sizeof(int); i++) {
			refc = refc_set[i];
			/* 1200 / (LSDC_PLL_REF_CLK_MHZ / refc)*/
			loopc_min = (1200 / LSDC_PLL_REF_CLK_MHZ) * refc;
			/* 3200 / (LSDC_PLL_REF_CLK_MHZ / refc)*/
			loopc_max = (3200 / LSDC_PLL_REF_CLK_MHZ) * refc;
			/* (loopc_min + loopc_max) / 2;*/
			loopc_mid = (2200 / LSDC_PLL_REF_CLK_MHZ) * refc;
			loopc_offset = -1;

			/* try each loopc */
			for (loopc = loopc_mid;
				(loopc <= loopc_max) && (loopc >= loopc_min);
				loopc += loopc_offset) {
				if (loopc_offset < 0)
					loopc_offset = -loopc_offset;
				else
					loopc_offset = -(loopc_offset+1);

				base = LSDC_PLL_REF_CLK_MHZ * 1000 / refc;

				pstdiv = loopc * base / pixclock;
				if ((pstdiv > 127) || (pstdiv < 1))
					continue;

				/*
				 * real_freq is float type which is not
				 * available, but read_freq * pstdiv is
				 * available.
				 */
				req_dvo = pixclock * pstdiv;
				real_dvo = loopc * base;
				precision = abs(real_dvo *
					PCLK_PRECISION_INDICATOR / req_dvo -
					PCLK_PRECISION_INDICATOR);

				if (precision < precision_req) {
					pll_config->l2_div = pstdiv;
					pll_config->l1_loopc = loopc;
					pll_config->l1_frefc = refc;
					return 1;
				}
			}
		}
	}

	return 0;
}


static unsigned int ls2k_cal_freq(unsigned int pixFreq,
				  struct pix_pll *pll_config)
{
	unsigned int pstdiv, loopc, frefc;
	unsigned long a, b, c;
	unsigned long min = 1000;

	for (pstdiv = 1; pstdiv < 64; pstdiv++) {
		a = (unsigned long)pixFreq * pstdiv;

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


static void ls2k_config_pll(void *pll_base, struct pix_pll *pll_cfg)
{
	unsigned long val;

	/* set sel_pll_out0 */
	val = readq(pll_base);
	val &= ~(1UL << 0);
	writeq(val, pll_base);

	/* pll_pd 1 */
	val = readq(pll_base);
	val |= (1UL << 19);
	writeq(val, pll_base);

	/* set_pll_param 0 */
	val = readq(pll_base);
	val &= ~(1UL << 2);
	writeq(val, pll_base);

	/* set new div ref, loopc, div out; clear old value first */
	val = (1 << 7) | (1L << 42) | (3 << 10) |
		((unsigned long)(pll_cfg->l1_loopc) << 32) |
		((unsigned long)(pll_cfg->l1_frefc) << 26);
	writeq(val, pll_base);
	writeq(pll_cfg->l2_div, pll_base + 8);

	/* set_pll_param 1 */
	val = readq(pll_base);
	val |= (1UL << 2);
	writeq(val, pll_base);

	/* pll_pd 0 */
	val = readq(pll_base);
	val &= ~(1UL << 19);
	writeq(val, pll_base);

	/* wait pll lock */
	while (!(readl(pll_base) & 0x10000))
		;
	/* set sel_pll_out0 1 */
	val = readq(pll_base);
	val |= (1UL << 0);
	writeq(val, pll_base);
}


void lsn_config_pll(struct loongson_drm_device *pLsDev,
		    int id, unsigned int pix_freq)
{
	unsigned int out;
	struct pix_pll pll_cfg;
	void *pll_base;

	if (pLsDev->chip == LSDC_CHIP_7A1000) {
		out = ls7a_cal_freq(pix_freq, &pll_cfg);

		if (id == 0)
			pll_base = (void *)TO_UNCAC(
				LSDC_7A1000_CFG_REG_BASE + LSDC_PIX_PLL_0);
		else if (id == 1)
			pll_base = (void *)TO_UNCAC(
				LSDC_7A1000_CFG_REG_BASE + LSDC_PIX_PLL_1);
		else {
			DRM_ERROR("PIX PLL no more than 2 in LS7A1000.\n");
			return;
		}

		ls7a_config_pll(pll_base, &pll_cfg);

		DRM_INFO("PIX-PLL%d, l2_div=%d, l1_loopc=%d, l1_frefc=%d\n",
		    id, pll_cfg.l2_div, pll_cfg.l1_loopc, pll_cfg.l1_frefc);

	} else if (pLsDev->chip == LSDC_CHIP_2K1000) {
		out = ls2k_cal_freq(pix_freq, &pll_cfg);

		if (id == 0)
			pll_base = (void *)TO_UNCAC(
				LSDC_2K1000_CFG_REG_BASE + LSDC_PIX_PLL_0);
		else if (id == 1)
			pll_base = (void *)TO_UNCAC(
				LSDC_2K1000_CFG_REG_BASE + LSDC_PIX_PLL_1);
		else
			DRM_WARN("PIX PLL no more than 2 in LS2K1000.\n");

		ls2k_config_pll(pll_base, &pll_cfg);

		DRM_INFO("PIX-PLL%d, l2_div=%d, l1_loopc=%d, l1_frefc=%d\n",
		    id, pll_cfg.l2_div, pll_cfg.l1_loopc, pll_cfg.l1_frefc);

	} else {
		DRM_WARN("%s: unknown loongson display controller\n",
			__func__);

		DRM_INFO("chip = %u.\n", pLsDev->chip);
	}
}
