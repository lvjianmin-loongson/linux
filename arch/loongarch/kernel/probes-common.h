/*
 * Copyright (C) 2016 Imagination Technologies
 * Author: Marcin Nowakowski <marcin.nowakowski@loongarch.com>
 * Copyright (C) 2020 Loongson Technology Corporation Limited
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __PROBES_COMMON_H
#define __PROBES_COMMON_H

#include <asm/inst.h>

static inline int __insn_has_delay_slot(const union loongarch_instruction insn)
{
	return 0;
}

#endif  /* __PROBES_COMMON_H */
