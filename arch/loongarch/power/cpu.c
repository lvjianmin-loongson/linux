// SPDX-License-Identifier: GPL-2.0
/*
 * Suspend support specific for loongarch.
 *
 * Licensed under the GPLv2
 *
 * Copyright (C) 2009 Lemote Inc.
 * Author: Hu Hongbing <huhb@lemote.com>
 *	   Wu Zhangjin <wuzhangjin@gmail.com>
 * Copyright (C) 2020 Loongson Technology Co., Ltd.
 */
#include <asm/sections.h>
#include <asm/fpu.h>
#include <lcsrintrin.h>

static u64 saved_crmd;
static u64 saved_prmd;
static u64 saved_cu;
static u64 saved_excfg;
struct pt_regs saved_regs;

void save_processor_state(void)
{
	saved_crmd = __lcsr_dcsrrd(LOONGARCH_CSR_CRMD);
	saved_prmd = __lcsr_dcsrrd(LOONGARCH_CSR_PRMD);
	saved_cu = __lcsr_dcsrrd(LOONGARCH_CSR_CU);
	saved_excfg = __lcsr_dcsrrd(LOONGARCH_CSR_EXCFG);

	if (is_fpu_owner())
		save_fp(current);
}

void restore_processor_state(void)
{
	__lcsr_dcsrwr(saved_crmd, LOONGARCH_CSR_CRMD);
	__lcsr_dcsrwr(saved_prmd, LOONGARCH_CSR_PRMD);
	__lcsr_dcsrwr(saved_cu, LOONGARCH_CSR_CU);
	__lcsr_dcsrwr(saved_excfg, LOONGARCH_CSR_EXCFG);

	if (is_fpu_owner())
		restore_fp(current);
}

int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = PFN_DOWN(__pa(&__nosave_begin));
	unsigned long nosave_end_pfn = PFN_UP(__pa(&__nosave_end));

	return	(pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}
