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
#include "ls3a.h"
#include <linux/random.h>
#include "ls3a_ext_irq.h"
#include "ls7a_irq.h"

#define ls3a_ext_irq_lock(s, flags)	spin_lock_irqsave(&s->lock, flags)
#define ls3a_ext_irq_unlock(s, flags)	spin_unlock_irqrestore(&s->lock, flags)

extern int kvm_vcpu_ioctl_interrupt(struct kvm_vcpu *vcpu,
			     struct kvm_mips_interrupt *irq);

/**
 * ext_irq_update_core()
 * @kvm: KVM structure pointer
 * @core: The number of the core that needs to be updated
 * @Register: number to be updated
 *
 * Route the status of the extended interrupt to the host CPU core.
 *
 */
void ext_irq_update_core(struct kvm *kvm,int core,int regnum)
{
	uint8_t ipnum[2];
	uint8_t level[4]={0,0,0,0};
	uint8_t ipmask[4]={0,0,0,0};
	uint64_t irq_state, ier;
	uint64_t ioisr;
	int nrcpus, i;

	struct kvm_mips_interrupt irq;
	struct ls3a_kvm_extirq *s = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *state = &(s->ls3a_ext_irq);

	nrcpus = atomic_read(&kvm->online_vcpus);
	if (core > (nrcpus - 1)) {
		core = 0;
	}

	for (i = 0; i < 2; i++) {
		ipnum[i] = __ffs(state->map.reg_u8[i + regnum*2] & 0xf);
		kvm_debug("core = %d,ipnum[%d] = 0x%x,regnum = %d\n",
						core, i, ipnum[i], regnum);
		if (ipnum[i] >= 0) {
			ipmask[ipnum[i]]++;
			ioisr = kvm->vcpus[core]->arch.core_ext_ioisr[regnum];
			irq_state = (ioisr >> (32 * (i % 2))) & 0xffffffffUL;
			ier = (state->en.reg_u64[regnum]>>(32*(i%2))) &
								0xffffffffUL;
			if ((irq_state & ier) != 0) {
				level[ipnum[i]]++;
			}
		}
	}
	for (i = 0; i<4; i++) {
		if (level[i] != 0) {
			kvm->arch.core_ip_mask[core][i] |=
						(0x1UL << (regnum + 1));
			irq.cpu = core;
			irq.irq = i+2;
			kvm_debug("vcpu %d,ip %d raise\n", core, i+2);
			kvm_vcpu_ioctl_interrupt(kvm->vcpus[core], &irq);
		} else if (ipmask[i] != 0) {
			kvm->arch.core_ip_mask[core][i] &=
						~(0x1UL << (regnum + 1));
			if (kvm->arch.core_ip_mask[core][i] == 0) {
				irq.cpu = core;
				irq.irq = -(i+2);
				kvm_debug("vcpu %d,ip %d down\n", core, i+2);
				kvm_vcpu_ioctl_interrupt(kvm->vcpus[core], &irq);
			}
		}
	}
}

/**
 *ext_irq_handler()
 * @kvm: KVM structure pointer
 * @irq: Irq number
 * @level: Interrupt level
 *
 * Extended interrupt handler
 */

void ext_irq_handler(struct kvm *kvm,int irq,int level)
{
	uint64_t reg_num,reg_bit;
	int nrcpus,core;
	struct ls3a_kvm_extirq *s = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *state = &(s->ls3a_ext_irq);

	reg_num = irq / 64;
	reg_bit = irq % 64;
	core= __ffs(state->core_map.reg_u8[irq] & 0xf);
	nrcpus = atomic_read(&kvm->online_vcpus);

	kvm_debug("ext_irq_handler:irq = %d,level = %d\n", irq, level);
	if (core > (nrcpus - 1)) {
		core = 0;
	}

	if (level == 1) {
		state->isr.reg_u64[irq / 64] |=  1ULL << (irq % 64);
		kvm->vcpus[core]->arch.core_ext_ioisr[reg_num] |=
							1ULL << reg_bit;
		ext_irq_update_core(kvm,core, reg_num);
	}
}

void msi_irq_handler(struct kvm *kvm, int irq, int level)
{
	ext_irq_handler(kvm, irq, level);
}

/**
 * ls3a_ext_intctl_read()
 * @kvm: KVM structure pointer
 * @addr: Register address
 * @size: The width of the register to be read.
 * @val: The pointer to the read result.
 *
 * Analog extended interrupt related register read.
 *
 */
uint64_t ls3a_ext_intctl_read(struct kvm *kvm, gpa_t addr, unsigned size,void *val)
{
	uint64_t offset,reg_count,node;
	struct ls3a_kvm_extirq *s = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *state = &(s->ls3a_ext_irq);

	offset = addr & 0xffff;
	node = (addr >> 44) & 0xff;

	if (offset & (size -1 )) {
		printk("%s(%d):unaligned address access %llx size %d \n",
			__FUNCTION__, __LINE__, addr, size);
		return 0;
	}

	if ((offset >= 0x1600) && (offset < 0x1620)) {
		if (size == 8) {
			reg_count = (offset-0x1600)/8;
			*(uint64_t *)val = state->en.reg_u64[reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1600) / 4;
			*(uint32_t *)val = state->en.reg_u32[reg_count];
		}
	} else if((offset >= 0x1680) && (offset < 0x16a0)) {
		if (size == 8) {
			reg_count = (offset - 0x1680) / 8;
			*(uint64_t *)val = state->bounce.reg_u64[reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1680) / 4;
			*(uint32_t *)val = state->bounce.reg_u32[reg_count];
		}
	} else if ((offset >= 0x1700) && (offset < 0x1720)) {
		if (size == 8) {
			reg_count = (offset - 0x1700) / 8;
			*(uint64_t *)val = state->isr.reg_u64[reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1700) / 4;
			*(uint32_t *)val = state->isr.reg_u32[reg_count];
		}
	} else if ((offset >= 0x1800) && (offset < 0x1820)) {
		if (size == 8) {
			reg_count = (offset - 0x1800) / 8;
			*(uint64_t *)val = state->core_isr.reg_u64[0][reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1800) / 4;
			*(uint32_t *)val = state->core_isr.reg_u32[0][reg_count];
		}
	} else if ((offset >= 0x1900) && (offset < 0x1920)) {
		if (size == 8) {
			reg_count = (offset - 0x1900) / 8;
			*(uint64_t *)val = state->core_isr.reg_u64[1][reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1900) / 4;
			*(uint32_t *)val = state->core_isr.reg_u32[1][reg_count];
		}
	} else if ((offset >= 0x1a00) && (offset < 0x1a20)) {
		if (size == 8) {
			reg_count = (offset - 0x1a00) / 8;
			*(uint64_t *)val = state->core_isr.reg_u64[2][reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1a00) / 4;
			*(uint32_t *)val = state->core_isr.reg_u32[2][reg_count];
		}
	} else if ((offset >= 0x1b00) && (offset < 0x1b20)) {
		if (size == 8) {
			reg_count = (offset - 0x1b00) / 8;
			*(uint64_t *)val = state->core_isr.reg_u64[3][reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1b00) / 4;
			*(uint32_t *)val = state->core_isr.reg_u32[3][reg_count];
		}
	} else if ((offset >= 0x14c0) && (offset < 0x14C8)) {
		if (size == 8) {
			*(uint64_t *)val = state->map.reg_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x14c0) / 4;
			*(uint32_t *)val = state->map.reg_u32[reg_count];
		}
	} else if ((offset >= 0x1c00) && (offset < 0x1cff)) {
		if (size == 8) {
			reg_count = (offset - 0x1c00) / 8;
			*(uint64_t *)val = state->core_map.reg_u64[reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x1c00)/4;
			*(uint32_t *)val = state->core_map.reg_u32[reg_count];
		}
	} else if ((offset >= 0x14a0) && (offset < 0x14bf)) {
		if (size == 8) {
			reg_count = (offset - 0x14a0) / 8;
			*(uint64_t *)val = state->node_type.reg_u64[reg_count];
		} else if (size == 4) {
			reg_count = (offset - 0x14a0) / 4;
			*(uint32_t *)val = state->node_type.reg_u32[reg_count];
		}
	} else if ((offset >= 0x0420) && (offset < 0x0428)) {
		if (size == 8) {
			*(uint64_t *)val = state->ext_en.reg_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x0420) / 4;
			*(uint32_t *)val = state->ext_en.reg_u32[reg_count];
		}
	}
	kvm_debug("ls3a_ext_intctl_read: addr=0x%llx,size=%d,val=0x%llx\n",
						addr, size, *(uint64_t *)val);
	return 0;
}

/**
 * ls3a_ext_intctl_write()
 * @kvm: KVM structure pointer
 * @addr: Register address
 * @size: The width of the register to be writen.
 * @val: Value to be written.
 *
 * Analog extended interrupt related register write.
 *
 */
int ls3a_ext_intctl_write(struct kvm *kvm , gpa_t addr, unsigned size, unsigned long val)
{
	uint64_t offset,val_data_u64,reg_count;
	uint32_t val_data_u32;
	uint8_t val_data_u8;

	struct ls3a_kvm_extirq *s = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *state = &(s->ls3a_ext_irq);

	offset = addr&0xffff;
	val_data_u64 = val;
	val_data_u32 = (uint32_t) (val & 0xffffffffUL);
	val_data_u8 = (uint8_t) (val & 0xffUL);

	kvm_debug("ls3a_ext_intctl_write: addr=0x%llx,size=%d,val=0x%lx\n",
							       addr, size, val);

	if (offset & (size -1 )) {
		printk("%s(%d):unaligned address access %llx size %d \n",
			__FUNCTION__, __LINE__, addr, size);
		return 0;
	}

	if ((offset >= 0x1600) && (offset < 0x1620)) {
		if (size == 8) {
			reg_count = (offset - 0x1600) /8;
			state->en.reg_u64[reg_count] = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1600) / 4;
			state->en.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1600);
			state->en.reg_u8[reg_count] = val_data_u8;
		}
		ext_irq_update_core(kvm, 0, (offset -0x1600) / 8);
	} else if ((offset >= 0x1680) && (offset < 0x16a0)) {
		if (size == 8) {
			reg_count = (offset - 0x1680) / 8;
			state->bounce.reg_u64[reg_count] = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1680) / 4;
			state->bounce.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1680);
			state->bounce.reg_u8[reg_count] = val_data_u8;
		}
	} else if ((offset >= 0x1700) && (offset < 0x1720)) {
		/*can not be writen*/
	} else if ((offset >= 0x1800) && (offset < 0x1820)) {
		if (size == 8) {
			reg_count = (offset - 0x1800) / 8;
			state->core_isr.reg_u64[0][reg_count] &= ~val_data_u64;
			state->isr.reg_u64[reg_count] &= ~val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1800) / 4;
			state->core_isr.reg_u32[0][reg_count] &= ~val_data_u32;
			state->isr.reg_u32[reg_count] &= ~val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1800);
			state->core_isr.reg_u8[0][reg_count] &= ~val_data_u8;
			state->isr.reg_u8[reg_count] &= ~val_data_u8;
		}
		ext_irq_update_core(kvm, 0, (offset - 0x1800) / 8);
	} else if ((offset >= 0x1900) && (offset < 0x1920)) {
		if (size == 8) {
			reg_count = (offset - 0x1900) / 8;
			state->core_isr.reg_u64[1][reg_count] &= ~val_data_u64;
			state->isr.reg_u64[reg_count] &= ~val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1900) / 4;
			state->core_isr.reg_u64[1][reg_count] &= ~val_data_u32;
			state->isr.reg_u32[reg_count] &= ~val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1900);
			state->core_isr.reg_u8[1][reg_count] &= ~val_data_u8;
			state->isr.reg_u8[reg_count] &= ~val_data_u8;
		}
		ext_irq_update_core(kvm, 1, (offset - 0x1900) / 8);
	} else if ((offset >= 0x1a00) && (offset < 0x1a20)) {
		if (size == 8) {
			reg_count = (offset - 0x1a00) / 8;
			state->core_isr.reg_u64[2][reg_count] &= ~val_data_u64;
			state->isr.reg_u64[reg_count] &= ~val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1a00) / 4;
			state->core_isr.reg_u64[2][reg_count] &= ~val_data_u32;
			state->isr.reg_u32[reg_count] &= ~val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1a00);
			state->core_isr.reg_u8[2][reg_count] &= ~val_data_u8;
			state->isr.reg_u8[reg_count] &= ~val_data_u8;
		}
		ext_irq_update_core(kvm, 2, (offset - 0x1a00) / 8);
	} else if ((offset >= 0x1b00) && (offset < 0x1b20)) {
		if (size == 8) {
			reg_count = (offset - 0x1b00) / 8;
			state->core_isr.reg_u64[3][reg_count] &= ~val_data_u64;
			state->isr.reg_u64[reg_count] &= ~val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1b00) / 4;
			state->core_isr.reg_u64[3][reg_count] &= ~val_data_u32;
			state->isr.reg_u32[reg_count] &= ~val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1b00);
			state->core_isr.reg_u8[3][reg_count] &= ~val_data_u8;
			state->isr.reg_u8[reg_count] &= ~val_data_u8;
		}
		ext_irq_update_core(kvm, 3, (offset - 0x1b00) / 8);
	} else if ((offset >= 0x14c0) && (offset < 0x14C8)) {
		if (size == 8) {
			state->map.reg_u64 = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x14c0) / 4;
			state->map.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x14c0);
			state->map.reg_u8[reg_count] = val_data_u8;
		}
	} else if ((offset >= 0x1c00) && (offset < 0x1cff)) {
		if (size == 8) {
			reg_count = (offset - 0x1c00) / 8;
			state->core_map.reg_u64[reg_count] = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x1c00) / 4;
			state->core_map.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x1c00);
			state->core_map.reg_u8[reg_count] = val_data_u8;
		}
	} else if ((offset >= 0x14a0) && (offset < 0x14bf)) {
		if (size == 8) {
			reg_count = (offset - 0x14a0) / 8;
			state->node_type.reg_u64[reg_count] = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x14a0) / 4;
			state->node_type.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x14a0);
			state->node_type.reg_u8[reg_count] = val_data_u8;
		}

	} else if ((offset >= 0x0420) && (offset < 0x0428)) {
		if (size == 8) {
			state->ext_en.reg_u64 = val_data_u64;
		} else if (size == 4) {
			reg_count = (offset - 0x0420) / 4;
			state->ext_en.reg_u32[reg_count] = val_data_u32;
		} else if (size == 1) {
			reg_count = (offset - 0x0420);
			state->ext_en.reg_u8[reg_count] = val_data_u8;
		}
	}else{
		WARN_ONCE(1,"Abnormal address access:addr 0x%llx,size %d\n",addr,size);
	}
	return 0;
}

/**
 * kvm_create_ls3a_ext_irq()
 * @kvm KVM structure pointer
 *
 * Create an extended interrupt resource instance for a virtual machine
 *
 * Returns: Extended interrupt structure pointer
 */
struct ls3a_kvm_extirq *kvm_create_ls3a_ext_irq(struct kvm *kvm)
{
	struct ls3a_kvm_extirq *s;

	s = kzalloc(sizeof(struct ls3a_kvm_extirq), GFP_KERNEL);
	if (!s)
		return NULL;
	spin_lock_init(&s->lock);
	s->kvm = kvm;
	return s;
}

int kvm_get_ls3a_extirq(struct kvm *kvm, struct kvm_loongson_ls3a_extirq_state *state)
{
	struct ls3a_kvm_extirq *v_extirq = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *extirq_state = &(v_extirq->ls3a_ext_irq);
	unsigned long flags;
	if (!v_extirq)
		return -EINVAL;

	ls3a_ext_irq_lock(v_extirq, flags);
	memcpy(state, extirq_state,
		sizeof(struct kvm_loongson_ls3a_extirq_state));
	ls3a_ext_irq_unlock(v_extirq, flags);
	kvm->stat.lsvz_kvm_get_ls3a_ext_irq++;
	return 0;
}

int kvm_set_ls3a_extirq(struct kvm *kvm, struct kvm_loongson_ls3a_extirq_state *state)
{
	struct ls3a_kvm_extirq *v_extirq = ls3a_ext_irqchip(kvm);
	struct kvm_ls3a_extirq_state *extirq_state = &(v_extirq->ls3a_ext_irq);
	unsigned long flags;
	if (!v_extirq)
		return -EINVAL;

	ls3a_ext_irq_lock(v_extirq, flags);
	memcpy(extirq_state, state,
		sizeof(struct kvm_loongson_ls3a_extirq_state));
	ls3a_ext_irq_unlock(v_extirq, flags);
	kvm->stat.lsvz_kvm_set_ls3a_ext_irq++;
	return 0;
}


int kvm_get_ls3a_ipmask(struct kvm *kvm, uint16_t *state)
{
	uint16_t *v_ipmask= (uint16_t *)(kvm->arch.core_ip_mask);
	if (!v_ipmask)
		return -EINVAL;

	memcpy(state, v_ipmask, sizeof(uint16_t) * 4 * KVM_MAX_VCPUS);
	kvm->stat.lsvz_kvm_get_ls3a_ipmask++;
	return 0;
}

int kvm_set_ls3a_ipmask(struct kvm *kvm, uint16_t *state)
{
	uint16_t *v_ipmask= (uint16_t *)(kvm->arch.core_ip_mask);
	if (!v_ipmask)
		return -EINVAL;

	memcpy(v_ipmask, state, sizeof(uint16_t) * 4 * KVM_MAX_VCPUS);
	kvm->stat.lsvz_kvm_set_ls3a_ipmask++;
	return 0;
}


void kvm_destroy_ls3a_ext_irq(struct ls3a_kvm_extirq *v_extirq)
{
	kfree(v_extirq);
}
