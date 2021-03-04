// SPDX-License-Identifier: GPL-2.0
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 MIPS Technologies, Inc.  All rights reserved.
 * Copyright (C) 2020 Loongson Technology Corporation Limited
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/addrspace.h>
#include <asm/fw/fw.h>

int fw_argc;
int *_fw_argv;
int *_fw_envp;

void __init fw_init_cmdline(void)
{
	int i;

	fw_argc = (fw_arg0 & 0x0000ffff);
	_fw_argv = (int *)fw_arg1;
	_fw_envp = (int *)fw_arg2;

	for (i = 1; i < fw_argc; i++) {
		strlcat(arcs_cmdline, fw_argv(i), COMMAND_LINE_SIZE);
		if (i < (fw_argc - 1))
			strlcat(arcs_cmdline, " ", COMMAND_LINE_SIZE);
	}
}

char * __init fw_getcmdline(void)
{
	return &(arcs_cmdline[0]);
}

char *fw_getenv(char *envname)
{
	char *result = NULL;

	if (_fw_envp != NULL) {
		/*
		 * Return a pointer to the given environment variable.
		 * YAMON uses "name", "value" pairs, while U-Boot uses
		 * "name=value".
		 */
		int i, yamon, index = 0;

		yamon = (strchr(fw_envp(index), '=') == NULL);
		i = strlen(envname);

		while (fw_envp(index)) {
			if (strncmp(envname, fw_envp(index), i) == 0) {
				if (yamon) {
					result = fw_envp(index + 1);
					break;
				} else if (fw_envp(index)[i] == '=') {
					result = fw_envp(index) + i + 1;
					break;
				}
			}

			/* Increment array index. */
			if (yamon)
				index += 2;
			else
				index += 1;
		}
	}

	return result;
}

unsigned long fw_getenvl(char *envname)
{
	unsigned long envl = 0UL;
	char *str;
	int tmp;

	str = fw_getenv(envname);
	if (str) {
		tmp = kstrtoul(str, 0, &envl);
		if (tmp)
			envl = 0;
	}

	return envl;
}
