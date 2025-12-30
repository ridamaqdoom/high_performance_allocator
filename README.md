A custom high-performance memory allocator written in C, inspired by modern production allocators such as jemalloc and tcmalloc.
The allocator focuses on low-latency allocation, scalability, and reduced syscall overhead using size classes, slab allocation, and thread-local caching.

This project demonstrates low-level systems knowledge including memory management, OS interaction (mmap), thread-local storage, and dynamic loader interposition.

**Overview**

Memory allocation is a critical performance bottleneck in many systems. General-purpose allocators must balance speed, fragmentation, and safety across diverse workloads. This project explores how modern allocators achieve performance by:

Avoiding linear searches

Minimizing kernel interactions

Reducing lock contention

Leveraging thread-local data

The allocator replaces the standard malloc family with a custom implementation that can be transparently injected into applications.

****Features**
**
Size-class based allocation (16B – 4KB)

Slab allocator to batch allocations and reduce mmap calls

Thread-local caches for near lock-free fast-path allocation

Global fallback allocator for cache refills

Direct mmap for large allocations

Transparent malloc interposition

Implements malloc, free, calloc, and realloc


****Allocation Strategy**
****Size Classes**

Small allocations are mapped to fixed size classes:
16, 32, 64, 128, 256, 512, 1024, 2048, 4096 bytes
Each size class maintains its own slab list, ensuring:
Constant-time allocation
Reduced fragmentation
Predictable memory layout


****Slab Allocation**
**
A slab is a contiguous memory region (64KB) obtained via mmap, subdivided into fixed-size blocks.
Benefits:
One syscall provides many allocations
Objects of the same size share cache lines

Improved locality and reuse
Each slab maintains an internal free list of blocks.
A slab is a contiguous memory region (64KB) obtained via mmap, subdivided into fixed-size blocks.

Benefits:

One syscall provides many allocation
Objects of the same size share cache lines
Improved locality and reuse
Each slab maintains an internal free list of blocks.

****Thread-Local Caching**
**
To eliminate lock contention:

Each thread maintains its own slab cache

Most allocations never acquire a mutex

Global allocator is only used when refilling a cache


****Malloc Interposition**
**
The allocator supports transparent replacement of the system allocator.

Mechanism

Builds as a shared library

Exports malloc, free, calloc, and realloc

Uses dynamic loader symbol resolution to override libc functions


**Limitations**

This allocator prioritizes performance and clarity over safety:

realloc uses a naive copy strategy

No double-free detection

No memory canaries or red zones

No statistics or introspection API

Not NUMA-aware

These limitations are intentional and documented trade-offs.
