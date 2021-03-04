/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * currently used for Loongson3A Virtual EXT interrupt.
 * Mapped into the guest kernel @ KVM_GUEST_COMMPAGE_ADDR.
 *
 * Copyright (C) 2019  Loongson Technologies, Inc.  All rights reserved.
 * Authors: Chen Zhu <zhuchen@loongson.cn>
 */
#ifndef __LS3A_KVM_EXT_IRQ_H
#define __LS3A_KVM_EXT_IRQ_H

#include <linux/mm_types.h>
#include <linux/hrtimer.h>
#include <linux/kvm_host.h>
#include <linux/spinlock.h>

#include <kvm/iodev.h>

typedef struct kvm_ls3a_extirq_state {
	union en{
		uint64_t reg_u64[4];
		uint32_t reg_u32[8];
		uint8_t reg_u8[32];
	}en;
	union bounce{
		uint64_t reg_u64[4];
		uint32_t reg_u32[8];
		uint8_t reg_u8[32];
	}bounce;
	union isr{
		uint64_t reg_u64[4];
		uint32_t reg_u32[8];
		uint8_t reg_u8[32];
	}isr;
	union core_isr{
		uint64_t reg_u64[4][4];
		uint32_t reg_u32[4][8];
		uint8_t reg_u8[4][32];
	}core_isr;
	union map{
		uint64_t reg_u64;
		uint32_t reg_u32[2];
		uint8_t reg_u8[8];
	}map;
	union core_map{
		uint64_t reg_u64[32];
		uint32_t reg_u32[64];
		uint8_t reg_u8[256];
	}core_map;
	union node_type{
		uint64_t reg_u64[4];
		uint32_t reg_u32[8];
		uint16_t reg_u16[16];
		uint8_t reg_u8[32];
	}node_type;
	union ext_en{
		uint64_t reg_u64;
		uint32_t reg_u32[2];
		uint8_t reg_u8[8];
	} ext_en;
}LS3AExtirqState;


struct ls3a_kvm_extirq{
	spinlock_t lock;
	struct kvm *kvm;
	struct kvm_io_device dev_ls3a_ext_irq;
	struct kvm_ls3a_extirq_state ls3a_ext_irq;
};


static inline struct ls3a_kvm_extirq *ls3a_ext_irqchip(struct kvm *kvm)
{
	return kvm->arch.v_extirq;
}

static inline int ls3a_extirq_in_kernel(struct kvm *kvm)
{
	int ret;

	ret = (ls3a_ext_irqchip(kvm) != NULL);
	return ret;
}


void ext_irq_update_core(struct kvm *kvm,int core,int regnum);
int ls3a_ext_intctl_write(struct kvm *kvm , gpa_t addr,
			  unsigned size, unsigned long val);
void ext_irq_handler(struct kvm *kvm,int irq,int level);
struct ls3a_kvm_extirq *kvm_create_ls3a_ext_irq(struct kvm *kvm);
int kvm_get_ls3a_extirq(struct kvm *kvm,
			struct kvm_loongson_ls3a_extirq_state *state);
int kvm_set_ls3a_extirq(struct kvm *kvm,
			struct kvm_loongson_ls3a_extirq_state *state);
int kvm_get_ls3a_ipmask(struct kvm *kvm, uint16_t *state);
int kvm_set_ls3a_ipmask(struct kvm *kvm, uint16_t *state);
void kvm_destroy_ls3a_ext_irq(struct ls3a_kvm_extirq *v_extirq);
void msi_irq_handler(struct kvm *kvm, int irq, int level);
#endif
