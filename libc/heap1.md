# heap1 — Simple Heap Allocator

`heap1.c` is a basic heap allocator without fixed-size pools.

## Design

- Uses one heap between `__heap_start` and `__heap_end`.
- Stores an 8-byte header per block on 32-bit systems.
- Aligns allocations to 8 bytes.
- `malloc()` uses first-fit search and splits large free blocks.
- `free()` merges neighboring free blocks.
- `realloc()` resizes in place or moves the data.
- Supports `calloc()`, heap statistics, and out-of-memory diagnostics.

## Limitations

Small allocations have relatively high overhead: an 8-byte allocation needs an 8-byte header. First-fit search may also become slow.

`heap2.c` improves this with fixed-size pools for 8, 16, 24, 32, and 48-byte allocations. See `heap2.md`.

## Build

Both allocators share `heap.h`. `heap1.c`.

```sh
rm -f qemu.elf
make HEAP=heap1 ramview
```

To use `heap2` (default):

```sh
rm -f qemu.elf
make HEAP=heap2 ramview
```

Remove `qemu.elf` when switching allocators to force relinking.
