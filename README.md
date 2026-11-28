# PerlOS — Yet Another Perl Computer

PerlOS is a lightweight, bare-metal operating environment developed by [@takesako](https://github.com/takesako). It starts an interactive Perl shell (REPL) directly on an Arm Cortex-M33 processor.

PerlOS combines a small kernel, microlibc, the ROMFS2 read-only filesystem, and a patched microperl runtime. On the Raspberry Pi Pico 2 (RP2350), it runs without Linux, newlib, or the Pico SDK.

The project supports two targets from a shared codebase:

- QEMU's `mps2-an505` machine for development and testing
- Raspberry Pi Pico 2 (RP2350) for deployment on real hardware

From Perl, you can access memory-mapped registers to control GPIO and other peripherals directly.

**Japanese hands-on tutorial:** [Building PerlOS in One Hour (Japanese)](https://zenn.dev/takesako/articles/8623cbed6c1bb6)

## Quick start

After installing the dependencies below, build and run PerlOS in QEMU:

```sh
make run
```

On the first run, the build downloads the required Perl 5.12.5 source files, applies the PerlOS patches, generates a ROMFS2 image, builds `qemu.elf`, and launches QEMU.

You should see:

```text
 ____           _  ___  ____
|  _ \ ___ _ __| |/ _ \/ ___|
| |_) / _ \ '__| | | | \___\
|  __/  __/ |  | | |_| |___) |
|_|   \___|_|  |_|\___/|____/
PerlOS v0.1.4 (microperl v5.12.5, NV=float, ROMFS2)
Arm MPS2+ AN505 / Cortex-M33 FPv5-SP-D16
```

At the `perl>` prompt, try:

```perl
help()
1 + 2
ls()
cat 'boot.pl'
meminfo()
do 'mandel.pl'
```

To exit PerlOS and close QEMU, enter `:quit` or press Ctrl-D.

## Perl script examples

The scripts in `root/` are packed into the ROMFS2 filesystem and can be run directly from the PerlOS REPL. Enter the following commands at the `perl>` prompt **inside QEMU**, not in your host shell.

For a numeric benchmark (the calculation may take some time):

```text
perl> do 'timethis.pl'
```

For interactive terminal animations and a simple game, try each script separately:

```text
perl> do 'aquarium.pl'
```

```text
perl> do 'lifegame.pl'
```

```text
perl> do 'matrix.pl'
```

```text
perl> do 'rocket.pl'
```

| Script | What it does | Controls |
|---|---|---|
| `aquarium.pl` | Animated aquarium with fish and bubbles | `q` to quit |
| `lifegame.pl` | Conway-style cellular automaton | Space to pause/resume, `r` to randomize, `q` to quit |
| `matrix.pl` | Falling text animation | `f` to speed up, `s` to slow down, `q` to quit |
| `rocket.pl` | Rocket game with moving obstacles | `w/a/s/d` or arrow keys to move, `q` to quit (collision also ends the game) |

Each interactive program returns to the `perl>` prompt when it ends. You can also re-run the startup script with `do 'boot.pl'`. The `pico2led.pl` example below requires a physical Raspberry Pi Pico 2.

## Requirements

Building PerlOS requires `make`, `perl`, `curl`, `tar`, `arm-none-eabi-gcc`, and `qemu-system-arm`. The source-fetch script uses GNU tar options to extract a selected set of Perl source files.

### macOS (Homebrew)

```sh
brew install make perl curl arm-none-eabi-gcc qemu
```

### Debian, Ubuntu, and WSL2

```sh
sudo apt update
sudo apt install make perl curl tar gcc-arm-none-eabi qemu-system-arm
```

### Optional tools

- `ffplay` (included in FFmpeg) for `make ramview`
- `zip` for `make zip`

Install FFmpeg with `brew install ffmpeg` on macOS or `sudo apt install ffmpeg` on Debian/Ubuntu.

## PerlOS files

```text
boot.S          Cortex-M33 vector table and startup code
kernel.c        kernel entry point and SysTick setup
main.c          application entry point (main())
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
example/        bare-metal C examples for main.c
script/         build, fetch, patch, and image tools
t/              test suites
perl-5.12.5/    fetched and patched Perl 5.12.5 source
```

## PerlOS architecture

1. **Perl scripts**
   `boot.pl`, `lib/strict.pm`, and application modules.

2. **microperl**
   Patched Perl 5.12.5 runtime with `NV=float`, ILP32, and no DynaLoader.

3. **`PerlOS::` extensions**
   XSUBs such as `load` and `store` for low-level memory and hardware access.

4. **microlibc**
   stdio, filesystem APIs, math, memory allocation, and other C runtime functions.

5. **ROMFS2**
   Read-only embedded filesystem built from `root/`.

6. **Kernel**
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

Perl scripts run on microperl, with `PerlOS::` XSUBs for hardware access and microlibc for C runtime support.

`root/` is packed into ROMFS2 at build time and accessed through microlibc filesystem APIs.

The kernel provides startup, timer, console, and low-level I/O for both QEMU and Raspberry Pi Pico 2.

## PerlOS internals

The boot sequence begins at the Cortex-M33 reset vector and ends at the Perl REPL.

### Boot

`boot.S` starts directly from the Cortex-M33 reset vector, enables the single-precision FPU, initializes `.data` and `.bss`, and enters the kernel.

The kernel configures SysTick and console I/O, then calls `main()`.

### microperl

PerlOS uses a patched `microperl` based on Perl 5.12.5.

The build uses:

- 32-bit ILP32
- `NV=float`
- no threads
- no DynaLoader
- no full PerlIO subsystem
- no complete POSIX runtime

The Cortex-M33 FPU is used with:

```text
-mfpu=fpv5-sp-d16
-mfloat-abi=hard
-fsingle-precision-constant
```

### microlibc

**microlibc** is a lightweight, freestanding C library used by PerlOS.

It provides:

- stdio, stdlib, string, ctype
- malloc, calloc, realloc, free
- float math functions
- time and SysTick support
- setjmp / longjmp
- Arm EABI helpers
- ROMFS2 file APIs

It is deliberately minimal and does not provide full ISO C or POSIX compatibility.

### ROMFS2

Files under `root/` are packed into ROMFS2 and linked directly into the firmware.

ROMFS2 provides a subset of file and directory operations, including:

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

Passing an RP2350 peripheral register address to these functions lets Perl control GPIO and other peripherals directly.

### Console

QEMU uses the MPS2+ AN505 UART. Semihosting is only used to terminate QEMU.

On the Raspberry Pi Pico 2, PerlOS uses its own USB CDC implementation, without depending on the Pico SDK or TinyUSB. USB processing runs on core 1.

## Build for Raspberry Pi Pico 2

Build the UF2 firmware using the memory-efficient `heap2` allocator:

```sh
make HEAP=heap2 pico2.uf2
```

The result is:

```text
pico2.uf2
```

Hold BOOTSEL while connecting the Pico 2 to your computer, then copy `pico2.uf2` to the mounted `RP2350` drive.

After the board restarts, connect to its USB CDC serial console:

```sh
perl script/usb-com.pl
```

Alternatively, use any compatible serial terminal to open the Pico 2 USB CDC port. On Windows, `perl script/usb-com.pl COM3` connects to COM3 (replace `COM3` with the actual port).

### Control Pico 2 hardware from Perl

The included example blinks the Pico 2 onboard LED using Perl alone:

```perl
do 'pico2led.pl'
```

The script uses `PerlOS::load()` and `PerlOS::store()` to configure GPIO hardware and toggle the output register.

This illustrates a central idea of PerlOS: a small set of memory-access primitives exposed to Perl is enough to control hardware without writing a new C driver for every experiment.

## Memory visualization

The `ramview` target starts QEMU with QMP (QEMU Machine Protocol) enabled. The `script/ramview.pl` helper periodically reads a region of QEMU RAM and displays it through `ffplay`, mapping each byte to a colored pixel.

```sh
make HEAP=heap1 ramview
```

This requires `ffplay` (from FFmpeg) and a local graphical display. The companion workshop describes use on macOS and Linux/WSL2; the available display environment may differ between systems.

## Other examples

The `example/` directory contains standalone C programs with their own `main()` functions:

- `echo.c` — `getchar` and `putchar` example
- `io.c` — timeout and blocking console I/O example
- `rocket.c` — keyboard-controlled emoji rocket game
- `poom.c` — DOOM-like game rendered in the terminal using Sixel
- `ime.c` — interactive romaji-to-Japanese input experiment
- `perl-shell.c` — PerlOS interactive Perl shell (REPL)

To run a C example, copy it to `main.c`, rebuild, and launch QEMU. For example, run the DOOM-like game first (a Sixel-capable terminal is required):

```sh
cp example/poom.c main.c
make run
```

Then try the Japanese input example:

```sh
cp example/ime.c main.c
make run
```

To return to the Perl REPL, restore the Perl shell entry point:

```sh
cp example/perl-shell.c main.c
make run
```

The companion workshop builds PerlOS incrementally: start with bare-metal C, add console I/O, microlibc, and ROMFS2, then integrate microperl. The examples demonstrate how the same bare-metal foundation can run a game, a simple Japanese input method, or the Perl REPL.

## PerlOS limitations

PerlOS deliberately leaves many conventional OS facilities out of scope.

- ROMFS2 is read-only.
- There is no process model, `fork()`, or executable loader.
- There is no network stack or socket API.
- There is no DynaLoader or support for loading shared modules.
- Perl threads are disabled.
- A full PerlIO subsystem is not available.
- Many POSIX APIs are unavailable or return `ENOSYS`.
- Perl numeric `NV` values are 32-bit `float`, so precision differs from normal desktop Perl builds.
- The C allocator is single-core and must not be called from interrupt handlers.
- The REPL provides basic line input but no command history or full cursor-editing support.
- Direct memory access through `PerlOS::load()` and `PerlOS::store()` is not memory-protected.

These constraints are part of the design: the project is intended to keep the implementation small enough to inspect and modify.

## License

PerlOS is triple-licensed under the MIT License, the BSD 3-Clause License, or the GNU General Public License version 1 or later.

The software is provided **"AS IS", WITHOUT WARRANTY OF ANY KIND**. Use at your own risk.

The following Perl-related files are licensed under the same terms as Perl itself:

```text
script/perl-get.sh
script/perl-patch.pl
script/perl-uconfig.pl
script/perl-uudmap.pl
```

These files may be redistributed and/or modified under either:

- the GNU General Public License, version 1 or later, or
- the Artistic License 1.0

Perl 5.12.5 source files downloaded or generated by the build remain under the original Perl licensing terms.

Modules under `root/lib/` that originate from CPAN remain subject to the licenses of their respective distributions.

Other third-party components, if any, remain subject to their respective licenses.

Copyright (c) 2026 Yoshinori Takesako — [@takesako](https://github.com/takesako)
