/*
 * Copyright (C) 2015 Imagination Technologies
 * Author: Alex Smith <alex.smith@imgtec.com>
 * Copyright (C) 2020 Loongson Technology Corporation Limited
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <asm/sgidefs.h>

#if _LOONGARCH_SIM != _LOONGARCH_SIM_ABILP64 && defined(CONFIG_64BIT)

/* Building 32-bit VDSO for the 64-bit kernel. Fake a 32-bit Kconfig. */
#undef CONFIG_64BIT
#define CONFIG_32BIT 1
#ifndef __ASSEMBLY__
#include <asm-generic/atomic64.h>
#endif
#endif

#ifndef __ASSEMBLY__

#include <asm/asm.h>
#include <asm/page.h>
#include <asm/vdso.h>

static inline const union loongarch_vdso_data *get_vdso_data(void)
{
	return (const union loongarch_vdso_data *)(get_vdso_base() - PAGE_SIZE);
}

#ifdef CONFIG_CLKSRC_LOONGARCH_GIC

static inline void __iomem *get_gic(const union loongarch_vdso_data *data)
{
	return (void __iomem *)data - PAGE_SIZE;
}

#endif /* CONFIG_CLKSRC_LOONGARCH_GIC */

#endif /* __ASSEMBLY__ */
