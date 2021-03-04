#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994, 95, 96, 2003 by Ralf Baechle
# DECStation modifications by Paul M. Antoine, 1996
# Copyright (C) 2002, 2003, 2004  Maciej W. Rozycki
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" cleaning up for this architecture.
#

archscripts: scripts_basic
	$(Q)$(MAKE) $(build)=arch/loongarch/tools elf-entry
	$(Q)$(MAKE) $(build)=arch/loongarch/boot/tools relocs

KBUILD_DEFCONFIG := 32r2el_defconfig
KBUILD_DTBS      := dtbs

#
# Select the object file format to substitute into the linker script.
#
64bit-tool-archpref	= loongarch64
32bit-bfd		= elf32-loongarch
64bit-bfd		= elf64-loongarch
32bit-emul		= elf32loongarch
64bit-emul		= elf64loongarch

ifdef CONFIG_64BIT
tool-archpref		= $(64bit-tool-archpref)
UTS_MACHINE		:= loongarch64
endif

ifneq ($(SUBARCH),$(ARCH))
  ifeq ($(CROSS_COMPILE),)
    CROSS_COMPILE := $(call cc-cross-prefix, $(tool-archpref)-linux-  $(tool-archpref)-linux-gnu-  $(tool-archpref)-unknown-linux-gnu-)
  endif
endif

ifdef CONFIG_FUNCTION_GRAPH_TRACER
  ifndef KBUILD_MCOUNT_RA_ADDRESS
    ifeq ($(call cc-option-yn,-mmcount-ra-address), y)
      cflags-y += -mmcount-ra-address -DKBUILD_MCOUNT_RA_ADDRESS
    endif
  endif
endif
cflags-y += $(call cc-option, -mno-check-zero-division)

ifdef CONFIG_64BIT
ld-emul			= $(64bit-emul)
vmlinux-64		= vmlinux

cflags-y		+= -mabi=lp64
endif

all-$(CONFIG_BOOT_ELF64)	:= $(vmlinux-64)
all-$(CONFIG_SYS_SUPPORTS_ZBOOT)+= vmlinuz

#
# GCC uses -G 0 -mabicalls -fpic as default.  We don't want PIC in the kernel
# code since it only slows down the whole thing.  At some point we might make
# use of global pointer optimizations but their use of $28 conflicts with
# the current pointer optimization.
#
# The DECStation requires an ECOFF kernel for remote booting, other LOONGARCH
# machines may also.  Since BFD is incredibly buggy with respect to
# crossformat linking we rely on the elf2ecoff tool for format conversion.
#
cflags-y			+= -G 0 -pipe
cflags-y			+= -msoft-float
cflags-y			+= -mlcsr
LDFLAGS_vmlinux			+= -G 0 -static -n -nostdlib
KBUILD_AFLAGS_KERNEL		+= -Wa,-mla-global-with-pcrel
KBUILD_CFLAGS_KERNEL		+= -Wa,-mla-global-with-pcrel
KBUILD_AFLAGS_MODULE		+= -Wa,-mla-global-with-abs
KBUILD_CFLAGS_MODULE		+= -fno-plt -Wa,-mla-global-with-abs,-mla-local-with-abs

ifeq ($(CONFIG_RELOCATABLE),y)
LDFLAGS_vmlinux			+= --emit-relocs
endif

#
# pass -msoft-float to GAS if it supports it.  However on newer binutils
# (specifically newer than 2.24.51.20140728) we then also need to explicitly
# set ".set hardfloat" in all files which manipulate floating point registers.
#
ifneq ($(call as-option,-Wa$(comma)-msoft-float,),)
	cflags-y		+= -DGAS_HAS_SET_HARDFLOAT -Wa,-msoft-float
endif

cflags-y += -ffreestanding

#
# We explicitly add the endianness specifier if needed, this allows
# to compile kernels with a toolchain for the other endianness. We
# carefully avoid to add it redundantly because gcc 3.3/3.4 complains
# when fed the toolchain default!
#
# Certain gcc versions up to gcc 4.1.1 (probably 4.2-subversion as of
# 2006-10-10 don't properly change the predefined symbols if -EB / -EL
# are used, so we kludge that here.  A bug has been filed at
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=29413.
#
# clang doesn't suffer from these issues and our checks against -dumpmachine
# don't work so well when cross compiling, since without providing --target
# clang's output will be based upon the build machine. So for clang we simply
# unconditionally specify -EB or -EL as appropriate.
#
ifeq ($(cc-name),clang)
cflags-$(CONFIG_CPU_LITTLE_ENDIAN)	+= -EL
else
undef-all += -ULOONGARCHEB -U_LOONGARCHEB -U__LOONGARCHEB -U__LOONGARCHEB__
undef-all += -ULOONGARCHEL -U_LOONGARCHEL -U__LOONGARCHEL -U__LOONGARCHEL__
predef-be += -DLOONGARCHEB -D_LOONGARCHEB -D__LOONGARCHEB -D__LOONGARCHEB__
predef-le += -DLOONGARCHEL -D_LOONGARCHEL -D__LOONGARCHEL -D__LOONGARCHEL__
cflags-$(CONFIG_CPU_LITTLE_ENDIAN)	+= $(shell $(CC) -dumpmachine |grep -q 'loongarch.*el-.*' || echo $(undef-all) $(predef-le))
endif

cflags-$(CONFIG_SB1XXX_CORELIS)	+= $(call cc-option,-mno-sched-prolog) \
				   -fno-omit-frame-pointer

# Some distribution-specific toolchains might pass the -fstack-check
# option during the build, which adds a simple stack-probe at the beginning
# of every function.  This stack probe is to ensure that there is enough
# stack space, else a SEGV is generated.  This is not desirable for LOONGARCH
# as kernel stacks are small, placed in unmapped virtual memory, and do not
# grow when overflowed.  Especially on SGI IP27 platforms, this check will
# lead to a NULL pointer dereference in _raw_spin_lock_irq.
#
# In disassembly, this stack probe appears at the top of a function as:
#    sd		zero,<offset>(sp)
# Where <offset> is a negative value.
#
cflags-y += -fno-stack-check

# For smartloongarch configurations, there are hundreds of warnings due to ISA overrides
# in assembly and header files. smartloongarch is only supported for LOONGARCH32r1 onwards
# and there is no support for 64-bit. Various '.set loongarch2' or '.set loongarch3' or
# similar directives in the kernel will spam the build logs with the following warnings:
# Warning: the `smartloongarch' extension requires LOONGARCH32 revision 1 or greater
# or
# Warning: the 64-bit LOONGARCH architecture does not support the `smartloongarch' extension
# Pass -Wa,--no-warn to disable all assembler warnings until the kernel code has
# been fixed properly.
loongarch-cflags				:= $(cflags-y)
ifeq ($(CONFIG_CPU_HAS_LSX),y)
toolchain-msa				:= $(call cc-option-yn,$(loongarch-cflags) -mhard-float -mfp64 -Wa$(comma)-mmsa)
cflags-$(toolchain-msa)			+= -DTOOLCHAIN_SUPPORTS_MSA
endif
toolchain-virt				:= $(call cc-option-yn,$(loongarch-cflags) -mvirt)
cflags-$(toolchain-virt)		+= -DTOOLCHAIN_SUPPORTS_VIRT
toolchain-crc				:= $(call cc-option-yn,$(loongarch-cflags) -Wa$(comma)-mcrc)
cflags-$(toolchain-crc)			+= -DTOOLCHAIN_SUPPORTS_CRC

#
# Firmware support
#
libs-y				+= arch/loongarch/fw/lib/

#
# Kernel compression
#
ifdef CONFIG_SYS_SUPPORTS_ZBOOT
COMPRESSION_FNAME		= vmlinuz
else
COMPRESSION_FNAME		= vmlinux
endif

#
# Board-dependent options and extra files
#
include arch/loongarch/Kbuild.platforms

ifdef CONFIG_PHYSICAL_START
load-y					= $(CONFIG_PHYSICAL_START)
endif

entry-y				= $(shell $(objtree)/arch/loongarch/tools/elf-entry vmlinux)
cflags-y			+= -I$(srctree)/arch/loongarch/include/asm/mach-generic
drivers-$(CONFIG_PCI)		+= arch/loongarch/pci/

KBUILD_AFLAGS	+= $(cflags-y)
KBUILD_CFLAGS	+= $(cflags-y)
KBUILD_CPPFLAGS += -DVMLINUX_LOAD_ADDRESS=$(load-y)
KBUILD_CPPFLAGS += -DDATAOFFSET=$(if $(dataoffset-y),$(dataoffset-y),0)

bootvars-y	= VMLINUX_LOAD_ADDRESS=$(load-y) \
		  VMLINUX_ENTRY_ADDRESS=$(entry-y) \
		  PLATFORM="$(platform-y)" \
		  ITS_INPUTS="$(its-y)"

ifdef CONFIG_64BIT
bootvars-y	+= ADDR_BITS=64
endif

# This is required to get dwarf unwinding tables into .debug_frame
# instead of .eh_frame so we don't discard them.
KBUILD_CFLAGS += -fno-asynchronous-unwind-tables

KBUILD_LDFLAGS		+= -m $(ld-emul)

ifdef CONFIG_LOONGARCH
CHECKFLAGS += $(shell $(CC) $(KBUILD_CFLAGS) -dM -E -x c /dev/null | \
	egrep -vw '__GNUC_(|MINOR_|PATCHLEVEL_)_' | \
	sed -e "s/^\#define /-D'/" -e "s/ /'='/" -e "s/$$/'/" -e 's/\$$/&&/g')
endif

OBJCOPYFLAGS		+= --remove-section=.reginfo

head-y := arch/loongarch/kernel/head.o

libs-y			+= arch/loongarch/lib/
#libs-y			+= arch/loongarch/math-emu/

# See arch/loongarch/Kbuild for content of core part of the kernel
core-y += arch/loongarch/

# suspend and hibernation support
drivers-$(CONFIG_PM)	+= arch/loongarch/power/

# boot image targets (arch/loongarch/boot/)
boot-y			:= vmlinux.bin
boot-y			+= vmlinux.ecoff
boot-y			+= vmlinux.srec
ifeq ($(shell expr $(load-y) \< 0xffffffff80000000 2> /dev/null), 0)
boot-y			+= uImage
boot-y			+= uImage.bin
boot-y			+= uImage.bz2
boot-y			+= uImage.gz
boot-y			+= uImage.lzma
boot-y			+= uImage.lzo
endif
boot-y			+= vmlinux.itb
boot-y			+= vmlinux.gz.itb
boot-y			+= vmlinux.bz2.itb
boot-y			+= vmlinux.lzma.itb
boot-y			+= vmlinux.lzo.itb

# compressed boot image targets (arch/loongarch/boot/compressed/)
bootz-y			:= vmlinuz
bootz-y			+= vmlinuz.bin
bootz-y			+= vmlinuz.ecoff
bootz-y			+= vmlinuz.srec
ifeq ($(shell expr $(zload-y) \< 0xffffffff80000000 2> /dev/null), 0)
bootz-y			+= uzImage.bin
endif

#
# The 64-bit ELF tools are pretty broken so at this time we generate 64-bit
# ELF files from 32-bit files by conversion.
#
quiet_cmd_64 = OBJCOPY $@
	cmd_64 = $(OBJCOPY) -O $(64bit-bfd) $(OBJCOPYFLAGS) $< $@
vmlinux.64: vmlinux
	$(call cmd,64)

all:	$(all-y) $(KBUILD_DTBS)

# boot
$(boot-y): $(vmlinux-64) FORCE
	$(Q)$(MAKE) $(build)=arch/loongarch/boot VMLINUX=$(vmlinux-64) \
		$(bootvars-y) arch/loongarch/boot/$@

ifdef CONFIG_SYS_SUPPORTS_ZBOOT
# boot/compressed
$(bootz-y): $(vmlinux-64) FORCE
	$(Q)$(MAKE) $(build)=arch/loongarch/boot/compressed \
		$(bootvars-y) 64bit-bfd=$(64bit-bfd) $@
else
vmlinuz: FORCE
	@echo '   CONFIG_SYS_SUPPORTS_ZBOOT is not enabled'
	/bin/false
endif


CLEAN_FILES += vmlinux.64

# device-trees
core-$(CONFIG_BUILTIN_DTB) += arch/loongarch/boot/dts/

%.dtb %.dtb.S %.dtb.o: | scripts
	$(Q)$(MAKE) $(build)=arch/loongarch/boot/dts arch/loongarch/boot/dts/$@

PHONY += dtbs
dtbs: scripts
	$(Q)$(MAKE) $(build)=arch/loongarch/boot/dts

PHONY += dtbs_install
dtbs_install:
	$(Q)$(MAKE) $(dtbinst)=arch/loongarch/boot/dts

install:
	$(Q)install -D -m 755 vmlinux $(INSTALL_PATH)/vmlinux-$(KERNELRELEASE)
ifdef CONFIG_SYS_SUPPORTS_ZBOOT
	$(Q)install -D -m 755 vmlinuz $(INSTALL_PATH)/vmlinuz-$(KERNELRELEASE)
endif
	$(Q)install -D -m 644 .config $(INSTALL_PATH)/config-$(KERNELRELEASE)
	$(Q)install -D -m 644 System.map $(INSTALL_PATH)/System.map-$(KERNELRELEASE)

archclean:
	$(Q)$(MAKE) $(clean)=arch/loongarch/boot
	$(Q)$(MAKE) $(clean)=arch/loongarch/boot/compressed
	$(Q)$(MAKE) $(clean)=arch/loongarch/boot/tools

define archhelp
	echo '  install              - install kernel into $(INSTALL_PATH)'
	echo '  vmlinux.ecoff        - ECOFF boot image'
	echo '  vmlinux.bin          - Raw binary boot image'
	echo '  vmlinux.srec         - SREC boot image'
	echo '  vmlinux.32           - 64-bit boot image wrapped in 32bits (IP22/IP32)'
	echo '  vmlinuz              - Compressed boot(zboot) image'
	echo '  vmlinuz.ecoff        - ECOFF zboot image'
	echo '  vmlinuz.bin          - Raw binary zboot image'
	echo '  vmlinuz.srec         - SREC zboot image'
	echo '  uImage               - U-Boot image'
	echo '  uImage.bin           - U-Boot image (uncompressed)'
	echo '  uImage.bz2           - U-Boot image (bz2)'
	echo '  uImage.gz            - U-Boot image (gzip)'
	echo '  uImage.lzma          - U-Boot image (lzma)'
	echo '  uImage.lzo           - U-Boot image (lzo)'
	echo '  uzImage.bin          - U-Boot image (self-extracting)'
	echo '  dtbs                 - Device-tree blobs for enabled boards'
	echo '  dtbs_install         - Install dtbs to $(INSTALL_DTBS_PATH)'
	echo
endef
