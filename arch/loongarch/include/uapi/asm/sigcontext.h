/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
* Copyright (C) 2020 Loongson Technology Corporation Limited
*
* Author: Hanlu Li <lihanlu@loongson.cn>
*/
#ifndef _UAPI_ASM_SIGCONTEXT_H
#define _UAPI_ASM_SIGCONTEXT_H

#include <linux/types.h>
#include <asm/sgidefs.h>
#include <asm/processor.h>

/* scalar FP context was used */
#define USED_FP			(1 << 0)

/* extended context was used, see struct extcontext for details */
#define USED_EXTCONTEXT		(1 << 1)

#include <linux/posix_types.h>
/*
 * Keep this struct definition in sync with the sigcontext fragment
 * in arch/loongarch/kernel/asm-offsets.c
 *
 */
struct sigcontext {
	__u64	sc_pc;
	__u64	sc_regs[32];
	__u32	sc_flags;

	__u32	sc_fcsr;
	__u32	sc_vcsr;
	__u64	sc_fcc;
	union fpureg	sc_fpregs[32] FPU_ALIGN;

#if defined(CONFIG_CPU_HAS_LBT)
	__u64	sc_scr[4];
#endif
	__u32	sc_reserved;
};

#endif /* _UAPI_ASM_SIGCONTEXT_H */
