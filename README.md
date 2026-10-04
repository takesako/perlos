# PerlOS — Yet Another Perl Computer

PerlOS is a custom OS developed by @takesako that boots straight into a Perl shell.

- PerlOS is a small bare-metal Perl environment for Arm Cortex-M33 embedded systems.
- PerlOS combines a small kernel, microlibc, ROMFS2, and microperl.
- PerlOS boots Raspberry Pi Pico 2 straight into Perl — no Linux, no newlib, no Pico SDK.

The same codebase runs on:

- QEMU `mps2-an505` for development and testing
- Raspberry Pi Pico 2 / RP2350 for real hardware

From Perl, you can access memory-mapped I/O and control hardware directly.

## Quick start

Run PerlOS on QEMU:

```sh
make run
```

The first build downloads the required Perl 5.12.5 sources, applies the PerlOS patches, builds ROMFS2 and `qemu.elf`, and starts QEMU.

You should see:

```text
qemu-system-arm -M mps2-an505 -display none -monitor none -serial stdio -semihosting-config enable=on,target=native -kernel qemu.elf
 ____           _  ___  ____
|  _ \ ___ _ __| |/ _ \/ ___|
| |_) / _ \ '__| | | | \___\
|  __/  __/ |  | | |_| |___) |
|_|   \___|_|  |_|\___/|____/
PerlOS 0.1.0 (microperl v5.12.5, NV=float, ROMFS2)
Arm MPS2+ AN505 / Cortex-M33 FPv5-SP-D16
```

Try:

```perl
help()
1 + 2
ls()
cat 'boot.pl'
meminfo()
do 'mandel.pl'
```

Exit with `:quit` or Ctrl-D.

## Requirements

PerlOS requires: `make`, `perl`, `curl`, `tar`, `arm-none-eabi-gcc`, and `qemu-system-arm`.

### macOS with Homebrew

```sh
brew install make perl curl gnu-tar arm-none-eabi-gcc qemu
```

### Debian / Ubuntu / WSL

```sh
sudo apt install make perl curl tar gcc-arm-none-eabi qemu-system-arm
```

### Optional tools

- `ffplay` for `make ramview`
- `zip` for `make zip`

## PerlOS files

```text
boot.S          Cortex-M33 reset vector and startup
kernel.c        kernel entry and SysTick setup
main.c          main() application entry
pico2.S         RP2350 boot metadata
qemu.ld         linker script for QEMU
pico2.ld        linker script for Raspberry Pi Pico 2

os/
  perlos.c      microperl embedding, PerlOS:: XSUBs, REPL
  romfs.c       ROMFS2 and read-only file APIs
  timer.c       1 ms system tick
  io_an505.c    QEMU AN505 UART backend
  io_pico2.c    Raspberry Pi Pico 2 USB CDC backend

libc/           microlibc implementation
root/           files embedded into ROMFS2
example/        bare-metal main.c example programs
script/         build, fetch, patch, and image tools
t/              test suites
perl-5.12.5/    fetched and patched Perl 5.12.5 source
```

## PerlOS architecture

1. **Perl scripts**
   `boot.pl`, `lib/strict.pm`, and application modules.

2. **microperl**
   Patched Perl 5.12.5 runtime with `NV=float`, ILP32, and no DynaLoader.

3. **PerlOS::**
   XSUBs such as `load` and `store` for low-level memory and hardware access.

4. **microlibc**
   stdio, filesystem APIs, math, memory allocation, and other C runtime functions.

5. **ROMFS2**
   Read-only embedded filesystem built from `root/`.

6. **kernel**
   Startup, timer, console, and low-level I/O support.

7. **I/O backends**
   QEMU MPS2+ AN505 and Raspberry Pi Pico 2 / RP2350.

```text
                       +---------------------------+
                       |       Perl scripts        |
                       | boot.pl, lib/strict.pm ...|
                       +-------------+-------------+
                                     |
                                     v
                    +---------------------------------+
                    |     microperl (Perl 5.12.5)     |
                    | NV=float / ILP32 / no DynaLoader|
                    +----------------+----------------+
                                     |
                         +-----------+-----------+
                         |                       |
                         v                       v
              +--------------------+   +--------------------+
              |      PerlOS::      |   |     microlibc      |
              |    load / store    |   | stdio, math, heap  |
              |       XSUBs        |   |     file I/O       |
              +---------+----------+   +---------+----------+
                        |                        ^
                        |                        |
                        |              open / read / stat
                        |                        |
                        |              +---------+----------+
                        |              |       ROMFS2       |
                        |              | read-only embedded |
                        |              |     filesystem     |
                        |              +---------+----------+
                        |                        ^
                        |                        |
                        |                    build time
                        |                        |
                        |              +---------+----------+
                        |              |       root/        |
                        |              |  boot.pl, lib/*.pm |
                        |              +--------------------+
                        |
                        +------------+
                                     |
                                     v
                       +---------------------------+
                       |          kernel           |
                       |  startup, timer, console  |
                       |        low-level I/O      |
                       +-------------+-------------+
                                     |
                         +-----------+-----------+
                         |                       |
                         v                       v
              +--------------------+   +--------------------+
              |  QEMU AN505 UART   |   |   RP2350 USB CDC   |
              | Cortex-M33 / FPU   |   |  Core 1 USB task   |
              +--------------------+   +--------------------+
```

Perl scripts run on microperl using `PerlOS::` XSUBs and microlibc.

`root/` is packed into ROMFS2 at build time and accessed through microlibc filesystem APIs.

The kernel provides startup, timer, console, and low-level I/O for both QEMU and Raspberry Pi Pico 2.

## PerlOS internals

PerlOS is built from the Cortex-M33 reset vector up to the Perl REPL.

### Boot

`boot.S` starts directly from the Cortex-M33 reset vector, enables the single-precision FPU, initializes `.data` and `.bss`, and enters the kernel.

The kernel sets up SysTick, console I/O, and starts `main()`.

### microperl

PerlOS uses a patched `microperl` based on Perl 5.12.5.

The build uses:

- 32-bit ILP32
- `NV=float`
- no threads
- no DynaLoader
- no PerlIO
- no POSIX layer

The Cortex-M33 FPU is used with:

```text
-mfpu=fpv5-sp-d16
-mfloat-abi=hard
-fsingle-precision-constant
```

### microlibc

**microlibc** is the small freestanding C library used by PerlOS.

It provides:

- stdio, stdlib, string, ctype
- malloc, calloc, realloc, free
- float math functions
- time and SysTick support
- setjmp / longjmp
- Arm EABI helpers
- ROMFS2 file APIs

It is intentionally small and does not aim for full ISO C or POSIX compatibility.

### ROMFS2

Files under `root/` are packed into ROMFS2 and linked directly into the firmware.

ROMFS2 provides read-only file APIs including:

```text
open
read
lseek
stat
chdir
```

Perl modules can be stored under `root/lib/`.

### Perl and hardware

PerlOS registers XSUBs directly without DynaLoader.

```perl
PerlOS::load($addr);                   # read 32-bit memory/MMIO
PerlOS::store($addr,$value);           # write 32-bit memory/MMIO
PerlOS::store($addr,$mask,SET_MASK);   # set bits in $mask
PerlOS::store($addr,$mask,CLR_MASK);   # clear bits in $mask
PerlOS::store($addr,$mask,XOR_MASK);   # toggle bits in $mask
```

These functions provide direct 32-bit memory-mapped I/O access.

Using a real RP2350 register address makes it possible to control GPIO and peripherals directly from Perl.

### Console

QEMU uses the MPS2+ AN505 UART. Semihosting is only used to terminate QEMU.

Raspberry Pi Pico 2 uses a built-in USB CDC implementation without Pico SDK or TinyUSB. USB processing runs on Core 1.

## Build for Raspberry Pi Pico 2

Build the UF2 image:

```sh
make pico2.uf2
```

The result is:

```text
pico2.uf2
```

Put the Pico 2 into UF2 boot mode and copy `pico2.uf2` to the mounted device.

Once the firmware starts, PerlOS exposes a USB CDC serial console:

```sh
perl script/usb-com.pl
```

You can also use a serial terminal that can open the Pico 2 CDC device.

### Control Pico 2 hardware from Perl

An included example blinks the Pico 2 LED entirely from Perl:

```perl
do 'pico2led.pl'
```

The script uses `PerlOS::load()` and `PerlOS::store()` to configure GPIO hardware and toggle the output register.

This is the core experiment behind PerlOS: once a few primitive memory operations cross the Perl/C boundary, surprisingly large parts of the hardware-facing system can be written in Perl itself.

## Memory visualization

`make ramview` starts QEMU with QMP enabled and runs `script/ramview.pl`, which periodically dumps QEMU RAM and sends it to `ffplay` as a live memory visualization.

```sh
make ramview
```

This currently assumes a Unix-like environment with `/dev/shm` and `ffplay`.

## Other examples

The `example/` directory contains `main()` programs:

- `echo.c` — `getchar` and `putchar` example
- `io.c` — timeout and blocking console I/O example
- `rocket.c` — emoji terminal mini game
- `ime.c` — interactive Japanese input experiment
- `poom.c` — DOOM-like game in the terminal using Sixel
- `perl-shell.c` — PerlOS REPL shell

Run an example by copying it to `main.c` and rebuilding:

```sh
cp example/ime.c main.c && make
```

These examples show how PerlOS is built incrementally: start with bare-metal C, add console I/O, microlibc, ROMFS2, and finally microperl.

## PerlOS limitations

PerlOS deliberately leaves many conventional OS facilities out of scope.

- ROMFS2 is read-only.
- There is no process model, `fork`, or executable loader.
- There is no network stack or socket API.
- There is no DynaLoader or shared module loading.
- Perl threads are disabled.
- PerlIO is disabled.
- Many POSIX calls are absent or return `ENOSYS`.
- Perl numeric `NV` values are 32-bit `float`, so precision differs from normal desktop Perl builds.
- The C allocator is single-core and must not be called from interrupt handlers.
- REPL line input is intentionally basic and has no history or full cursor editing.
- Arbitrary memory access through `PerlOS::load/store` has no protection.

These constraints are part of the design: the project is intended to keep the implementation small enough to inspect and modify.
