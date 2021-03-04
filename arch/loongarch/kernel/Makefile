# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux/LOONGARCH kernel.
#

extra-y		:= head.o vmlinux.lds

obj-y		+= cmpxchg.o cpu-probe.o elf.o entry.o genex.o idle.o irq.o \
		   process.o prom.o ptrace.o reset.o setup.o signal.o \
		   syscall.o time.o topology.o traps.o unaligned.o \
		   vdso.o cacheinfo.o stable_counter.o syscall_table.o \
		   virt.o

ifdef CONFIG_FUNCTION_TRACER
CFLAGS_REMOVE_ftrace.o = -pg
CFLAGS_REMOVE_early_printk.o = -pg
CFLAGS_REMOVE_perf_event.o = -pg
CFLAGS_REMOVE_perf_event_loongarchxx.o = -pg
endif

obj-$(CONFIG_STACKTRACE)	+= stacktrace.o
obj-$(CONFIG_MODULES)		+= module.o

obj-$(CONFIG_FTRACE_SYSCALLS)	+= ftrace.o
obj-$(CONFIG_FUNCTION_TRACER)	+= mcount.o ftrace.o

sw-y				:= switch.o
obj-y				+= $(sw-y)

obj-$(CONFIG_CPU_HAS_FPU)	+= fpu.o
obj-$(CONFIG_CPU_HAS_LASX)     += lasx.o

obj-y				+= acpi/

obj-$(CONFIG_SMP)		+= smp.o
obj-$(CONFIG_SMP_UP)		+= smp-up.o

obj-$(CONFIG_LOONGARCH_CPS_NS16550)	+= cps-vec-ns16550.o

obj-$(CONFIG_KPROBES)		+= kprobes.o
obj-$(CONFIG_64BIT)		+= scall64-64.o

obj-$(CONFIG_KGDB)		+= kgdb.o
obj-$(CONFIG_PROC_FS)		+= proc.o
obj-$(CONFIG_MAGIC_SYSRQ)	+= sysrq.o

obj-$(CONFIG_I8253)		+= i8253.o

obj-$(CONFIG_RELOCATABLE)	+= relocate.o

obj-$(CONFIG_KEXEC)		+= machine_kexec.o relocate_kernel.o crash.o
obj-$(CONFIG_CRASH_DUMP)	+= crash_dump.o
obj-$(CONFIG_EARLY_PRINTK)	+= early_printk.o
obj-$(CONFIG_EARLY_PRINTK_8250)	+= early_printk_8250.o
obj-$(CONFIG_SPINLOCK_TEST)	+= spinlock_test.o
obj-$(CONFIG_LOONGARCH_MACHINE)	+= loongarch_machine.o

obj-$(CONFIG_HAVE_STD_PC_SERIAL_PORT)	+= 8250-platform.o

obj-$(CONFIG_PERF_EVENTS)	+= perf_event.o
obj-$(CONFIG_HW_PERF_EVENTS)	+= perf_event_loongarchxx.o

obj-$(CONFIG_UPROBES)		+= uprobes.o

obj-$(CONFIG_CPU_PM)		+= pm.o

#
# DSP ASE supported for LOONGARCH32 or LOONGARCH64 Release 2 cores only. It is not
# safe to unconditionnaly use the assembler -mdsp / -mdspr2 switches
# here because the compiler may use DSP ASE instructions (such as lwx) in
# code paths where we cannot check that the CPU we are running on supports it.
# Proper abstraction using HAVE_AS_DSP and macros is done in
# arch/loongarch/include/asm/loongarchregs.h.
#
ifeq ($(CONFIG_CPU_LOONGARCHR2), y)
CFLAGS_DSP 			= -DHAVE_AS_DSP

CFLAGS_signal.o			= $(CFLAGS_DSP)
CFLAGS_process.o		= $(CFLAGS_DSP)
CFLAGS_branch.o			= $(CFLAGS_DSP)
CFLAGS_ptrace.o			= $(CFLAGS_DSP)
endif

CPPFLAGS_vmlinux.lds		:= $(KBUILD_CFLAGS)
