/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
* Copyright (C) 2020 Loongson Technology Corporation Limited
*
* Author: Hanlu Li <lihanlu@loongson.cn>
*/
#ifndef _UAPI_ASM_PTRACE_H
#define _UAPI_ASM_PTRACE_H

#include <linux/types.h>

/* For PTRACE_{POKE,PEEK}USR. 0 - 31 are GPRs, 32 is PC.  */
#define GPR_BASE	0
#define GPR_NUM		32
#define GPR_END		(GPR_BASE + GPR_NUM - 1)
#define PC		(GPR_END + 1)

#endif /* _UAPI_ASM_PTRACE_H */
