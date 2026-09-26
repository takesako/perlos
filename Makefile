QEMU      := qemu-system-arm
QEMU_ARGS := -M mps2-an505 -display none -monitor none -serial stdio \
             -semihosting-config enable=on,target=native
CC        := arm-none-eabi-gcc
CFLAGS    := -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard \
             -nostdinc -Ilibc -Ios -Iperl-5.12.5 -std=gnu11 -Os -Wall \
             -Wextra -Wno-misleading-indentation -Wno-unused-parameter \
             -Werror=implicit-function-declaration \
             -ffreestanding -fno-builtin -fno-stack-protector \
             -fno-unwind-tables -fno-asynchronous-unwind-tables \
             -ffunction-sections -fdata-sections -nostdlib -DROMFS
LDFLAGS   := -Wl,--gc-sections
PERL      := perl
PERL_DIR  := perl-5.12.5
PERL_READY:= $(PERL_DIR)/uudmap.h
PERL_DEFS := -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DNO_MATHOMS \
             -DPICOPERL_NV_FLOAT -DPERL_USE_SAFE_PUTENV \
             -DPERL_EXTERNAL_GLOB -DPERL_ARENA_SIZE=1024
PERL_OPTS := -fsingle-precision-constant -Wno-unused-variable \
             -Wno-unused-but-set-variable -Wno-implicit-fallthrough \
             -Wno-maybe-uninitialized -Wno-address
PERL_CORE := av deb doio doop dump globals gv hv locale mg mro numeric \
             op pad perl perlapi perlio perly pp pp_ctl pp_hot pp_pack \
             pp_sort pp_sys reentr regcomp regexec run scope sv taint \
             toke universal utf8 util
LIBC_CORE := aeabi errno math setjmp stdio stdlib string time heap
LIBC_OBJS := $(addprefix libc/,$(addsuffix .o,$(LIBC_CORE)))
PERL_OBJS := $(addprefix $(PERL_DIR)/,$(addsuffix .o,$(PERL_CORE)))
PERL_SRCS := $(PERL_OBJS:.o=.c)
OS_OBJS   := os/romfs.o os/perlos.o
ROMFS_IMG := os/romfs_image.o
ROM_FILES := $(shell find root -type f)
KERNEL    := boot.S kernel.c os/timer.c main.c
QEMU_SYS  := $(KERNEL) os/io_an505.c
PICO_SYS  := $(KERNEL) os/io_pico2.c pico2.S
TESTS     := $(wildcard t/*.t)
TEST_ELF  := $(TESTS:.t=.elf)

all: qemu.elf pico2.uf2

# libc

libc/%.o: libc/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# microperl

$(PERL_SRCS): | $(PERL_READY)
$(PERL_READY):
	@test -d "$(PERL_DIR)" || sh script/perl-get.sh
	chmod 644 $(PERL_DIR)/*
	cd $(PERL_DIR) && \
		perl ../script/perl-patch.pl && \
		perl ../script/perl-uconfig.pl && \
		perl ../script/perl-uudmap.pl
	@touch $@

$(PERL_DIR)/%.o: $(PERL_DIR)/%.c
	$(CC) $(CFLAGS) $(PERL_DEFS) $(PERL_OPTS) -c $< -o $@

# PerlOS

os/romfs.o: os/romfs.c os/romfs.h
	$(CC) $(CFLAGS) -c $< -o $@

os/perlos.o: os/perlos.c os/perlos.h
	$(CC) $(CFLAGS) $(PERL_DEFS) -c $< -o $@

# ROMFS image

os/romfs_image.rom: $(ROM_FILES) script/mkromfs.pl
	$(PERL) script/mkromfs.pl root $@

os/romfs_image.c: os/romfs_image.rom script/mkromfs.pl
	$(PERL) script/mkromfs.pl --carray $< $@ romfs_image

os/romfs_image.o: os/romfs_image.c
	$(CC) $(CFLAGS) -c $< -o $@

# QEMU / Arm MPS2+ AN505

qemu.elf: qemu.ld $(QEMU_SYS) $(LIBC_OBJS) $(PERL_OBJS) $(OS_OBJS) $(ROMFS_IMG)
	$(CC) $(CFLAGS) -DQEMU -T qemu.ld $(QEMU_SYS) \
		$(LIBC_OBJS) $(PERL_OBJS) $(OS_OBJS) $(ROMFS_IMG) \
		$(LDFLAGS) -o $@

run: qemu.elf
	$(QEMU) -version
	$(QEMU) $(QEMU_ARGS) -kernel $<

# Raspberry Pi Pico 2

pico2.elf: pico2.ld $(PICO_SYS) $(LIBC_OBJS) $(PERL_OBJS) $(OS_OBJS) $(ROMFS_IMG)
	$(CC) $(CFLAGS) -DPICO2 -T pico2.ld $(PICO_SYS) \
		$(LIBC_OBJS) $(PERL_OBJS) $(OS_OBJS) $(ROMFS_IMG) \
		$(LDFLAGS) $(LDLIBS) -o $@

pico2.uf2: pico2.elf
	$(PERL) script/picotool.pl uf2 convert $< $@ --family 0xe48bff59

# Tests

t/%.elf: t/%.t os/io.h os/timer.h qemu.ld $(QEMU_SYS) $(LIBC_OBJS) os/romfs.o
	$(CC) $(CFLAGS) -DQEMU -DTEST -T qemu.ld \
		$(QEMU_SYS) -x c $< -x none \
		$(LIBC_OBJS) os/romfs.o \
		$(LDFLAGS) $(LDLIBS) -o $@

test: $(TEST_ELF)
	@test -n "$(TESTS)" || { echo "No tests in t/*.t"; exit 1; }
	@for t in $(TEST_ELF); do \
		echo "=== $$t ==="; \
		$(QEMU) $(QEMU_ARGS) -kernel $$t || exit $$?; \
	done
	@echo "All tests passed."

clean:
	rm -f *.elf *.uf2 t/*.elf libc/*.o perl-5.12.5/*.o os/*.o
	rm -f os/romfs_image.rom os/romfs_image.c

.PHONY: all run test clean
