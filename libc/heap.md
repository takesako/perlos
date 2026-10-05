# Heap Allocator Design

PerlOS uses a small custom heap allocator designed for constrained embedded systems.

## Goal

The original allocator used a generic heap for every allocation.  
Each block required an 8-byte header, which became expensive because Perl creates about 10,000 small objects.

Profiling showed that most live allocations are small:

-  8 bytes
- 16 bytes
- 24 bytes
- 32 bytes
- 48 bytes

These size classes account for about 95% of live allocations in the tested Perl workloads.

## Design

PerlOS now supports a hybrid allocator:

```text
heap
+-------------------------+
| fixed-size object pools |
| 8 / 16 / 24 / 32 / 48   |
+-------------------------+
| generic heap            |
| first-fit allocator     |
+-------------------------+
```

Small allocations use fixed-size pools with no per-object allocator header.

Larger allocations, or allocations made after a pool is full, fall back to the existing generic allocator.

## Pool Configuration

Current default pool sizes:

| Object size | Slots |
|---------:|-----:|
|  8 bytes |  512 |
| 16 bytes |  768 |
| 24 bytes | 4640 |
| 32 bytes | 2640 |
| 48 bytes | 1200 |

The pools reserve about 270 KB of the configured heap.

A free slot stores its free-list pointer inside the slot itself, so no additional per-object metadata is required.

## Build Options

Enable the small-object allocator:

```sh
-DHEAP_SMALL_POOL
```

Enable allocation profiling:

```sh
-DHEAP_PROFILE
```

Both can be enabled together:

```sh
-DHEAP_SMALL_POOL -DHEAP_PROFILE
```

Without `HEAP_SMALL_POOL`, the original generic allocator is used.

## Results

On QEMU MPS2-AN505 with a Perl Benchmark workload:

| Metric | Generic heap | Hybrid allocator |
|---|---:|---:|
| Peak tracked memory | ~541 KB | ~464 KB |
| Block-header overhead | ~80 KB | ~4.5 KB |
| Header overhead reduction | - | ~94% |

The small-object pools eliminate roughly 75 KB of per-object headers in the measured workload.

## Generic Allocator

The fallback allocator keeps the existing design:

- 8-byte alignment
- first-fit search
- block splitting
- adjacent free-block coalescing
- in-place `realloc()` when possible

## Profiling

`HEAP_PROFILE` records:

- current allocation histogram
- histogram at peak heap usage
- maximum simultaneous count for each size class
- heap usage and fragmentation statistics

This profiling data was used to choose the fixed pool sizes.

## Design Principle

The allocator intentionally stays simple:

1. Measure real allocation patterns.
2. Optimize only the dominant size classes.
3. Keep the generic allocator as a fallback.
4. Make optimizations optional with compile-time flags.
