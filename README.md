# libmem

> **Repo:** [github.com/Ertugrul-Pakdamar/libmem](https://github.com/Ertugrul-Pakdamar/libmem)

A standalone C11 allocator library providing three complementary allocator types:
**pool**, **arena**, and **slab**. All allocators operate on caller-supplied
buffers — no internal `malloc` is ever performed.

Designed for embedded systems, real-time environments, and any project that
needs deterministic memory behaviour without dynamic allocation.

---

## Allocator Summary

| Allocator | Alloc | Free | Best for |
|---|---|---|---|
| Pool | O(1) | O(1) | Fixed-size objects (nodes, messages, handles) |
| Arena | O(1) | O(1) reset / rewind | Short-lived scratch memory, frame allocations |
| Slab | O(n pools) | O(n pools) | Variable-size objects with known size classes |

---

## Repository Layout

```
libmem/
├── src/
│   ├── pool.c               fixed-size block pool
│   ├── arena.c              linear bump allocator
│   └── slab.c               multi-pool variable-size allocator
├── include/
│   └── libmem.h             public API  ← start here
├── examples/
│   ├── 01_pool.c            pool alloc / free / reuse
│   ├── 02_arena.c           bump allocation, mark/rewind, reset
│   └── 03_slab.c            multi-pool selection, over-size rejection
├── build/                   generated — not committed
├── Makefile
├── README.md
└── CONTRIBUTING.md
```

---

## Getting Started

```bash
git clone https://github.com/Ertugrul-Pakdamar/libmem.git
cd libmem
make all     # produces libmem.a
```

### Available targets

```
make all        Build libmem.a  (release)
make debug      Build libmem.a with -DLIBMEM_DEBUG -g
make examples   Build all programs in examples/
make clean      Remove object files and example binaries
make fclean     clean + remove libmem.a
make re         fclean + all
```

---

## Examples

Ready-to-run programs live in `examples/`. Build them all with:

```bash
make examples
```

| Binary | Source | What it shows |
|---|---|---|
| `examples/bin/01_pool` | `examples/01_pool.c` | Pool alloc/free, exhaustion, reuse |
| `examples/bin/02_arena` | `examples/02_arena.c` | Bump allocation, mark/rewind, reset, introspection |
| `examples/bin/03_slab` | `examples/03_slab.c` | Multi-pool slab, automatic pool selection, over-size rejection |

---

## Pool Allocator

Maintains an intrusive free-list inside a caller-supplied buffer. Every block
is the same size. Both alloc and free are O(1).

**Constraints** checked at `pool_init()` time:
- `block_size` ≥ `sizeof(void*)` and must be a multiple of `sizeof(void*)`.
- `buffer` must be aligned to at least `sizeof(void*)`.
- `block_count` must be > 0.

On any violation the pool is left in a safe but unusable state (`free_head == NULL`);
debug builds additionally print a diagnostic to `stderr`.

```c
#include "libmem.h"

/* Buffer must be aligned to at least sizeof(void*). */
static unsigned char buf[sizeof(MyStruct) * 32];

mem_pool_t pool;
pool_init(&pool, buf, sizeof(MyStruct), 32);

MyStruct *obj = pool_alloc(&pool);  /* NULL if pool is exhausted */
/* ... use obj ... */
pool_free(&pool, obj);

/* Capacity introspection */
printf("in use: %zu / %zu\n", pool_used(&pool), pool.total_blocks);
```

---

## Arena Allocator

Bump allocator: advances a cursor on every allocation. Every allocation is
automatically aligned to `LIBMEM_MAX_ALIGN` (platform's `max_align_t`).
Use `arena_alloc_aligned()` for a custom power-of-2 alignment.

Individual frees are not supported. Use `arena_reset()` to reclaim everything
at once, or `arena_mark()` / `arena_rewind()` for lightweight partial reclamation.

```c
static unsigned char buf[4096];

mem_arena_t arena;
arena_init(&arena, buf, sizeof(buf));

void *a = arena_alloc(&arena, 64);   /* NULL if out of space */
void *b = arena_alloc(&arena, 128);

/* --- Mark / rewind for temporary allocations --- */
mem_arena_mark_t mark = arena_mark(&arena);

void *tmp = arena_alloc(&arena, 256);  /* temporary scratch */
/* ... use tmp ... */

arena_rewind(&arena, mark);           /* discard tmp in O(1) */

/* --- Capacity introspection --- */
printf("used: %zu  remaining: %zu\n", arena_used(&arena), arena_remaining(&arena));

arena_reset(&arena);                  /* releases everything at once */
```

---

## Slab Allocator

Manages up to `SLAB_MAX_POOLS` (8) pools of increasing block size.
`slab_alloc()` picks the smallest pool that can satisfy the request.

```c
static unsigned char small_buf[64];   /* 8 × 8-byte blocks  */
static unsigned char large_buf[256];  /* 8 × 32-byte blocks */

mem_slab_config_t cfgs[] = {
    { small_buf,  8,  8 },
    { large_buf, 32,  8 },
};

mem_slab_t slab;
slab_init(&slab, cfgs, 2);

void *p = slab_alloc(&slab, 20);  /* uses the 32-byte pool */
slab_free(&slab, p);
```

---

## API Reference

### Pool

```c
/* Initialize a pool allocator over a caller-supplied buffer.
   block_size >= sizeof(void*), a multiple of sizeof(void*), block_count > 0. */
void   pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);

/* Allocate one block. Returns NULL if exhausted or misconfigured. */
void  *pool_alloc(mem_pool_t *pool);

/* Return a block to the pool. Ignores NULL and out-of-range pointers.
   Debug builds detect and report double-frees. */
void   pool_free(mem_pool_t *pool, void *ptr);

/* Introspection */
size_t pool_used(const mem_pool_t *pool);       /* blocks currently in use  */
size_t pool_remaining(const mem_pool_t *pool);  /* blocks available to alloc */
```

### Arena

```c
/* Initialize a bump allocator over a caller-supplied buffer. */
void   arena_init(mem_arena_t *arena, void *buffer, size_t size);

/* Allocate alloc_size bytes aligned to LIBMEM_MAX_ALIGN. Returns NULL if
   out of space or if alloc_size == 0. */
void  *arena_alloc(mem_arena_t *arena, size_t alloc_size);

/* Same as arena_alloc() but with an explicit power-of-2 alignment. */
void  *arena_alloc_aligned(mem_arena_t *arena, size_t alloc_size, size_t align);

/* Release all arena allocations by resetting the cursor. */
void   arena_reset(mem_arena_t *arena);

/* Save / restore the allocation cursor for partial reclamation. */
mem_arena_mark_t  arena_mark(const mem_arena_t *arena);
void              arena_rewind(mem_arena_t *arena, mem_arena_mark_t mark);

/* Introspection */
size_t arena_used(const mem_arena_t *arena);       /* bytes consumed (incl. padding) */
size_t arena_remaining(const mem_arena_t *arena);  /* bytes still available          */
```

### Slab

```c
/* Initialize a slab from an array of pool configurations. */
void   slab_init(mem_slab_t *slab, mem_slab_config_t *configs, size_t count);

/* Allocate from the smallest pool that fits size. Returns NULL on failure. */
void  *slab_alloc(mem_slab_t *slab, size_t size);

/* Return an allocation to its owning pool. */
void   slab_free(mem_slab_t *slab, void *ptr);
```

---

## Debug Mode

Compile the library with `-DLIBMEM_DEBUG` (or use `make debug`) to enable:

| Feature | Description |
|---|---|
| **Double-free detection** | `pool_free()` scans the free-list (O(n)) and reports to `stderr` |
| **Allocation statistics** | `alloc_count` and `peak_usage` / `peak_used` fields in each struct |
| **Init diagnostics** | `pool_init()` prints a message for each violated constraint |
| **Stat helpers** | `arena_print_stats()` and `pool_print_stats()` print a summary to `stderr` |

```bash
make debug          # build libmem.a with -DLIBMEM_DEBUG -g
```

> **Warning:** struct sizes differ between debug and release builds.
> Never link debug-compiled objects with release-compiled objects of this library.

### Example output

```
[libmem/pool] total=4  used=2  free=2  alloc_count=2  peak_used=2
[libmem/pool] double-free detected: ptr=0x403040 pool=0x7ffe02902100
[libmem/arena] capacity=128  used=48  remaining=80  alloc_count=3  peak_usage=56
```

---

## Integration into Your Project

```bash
# Option A — copy files
cp libmem.a          /your/project/lib/
cp include/libmem.h  /your/project/include/

# Link (release)
cc main.c -o app -Iinclude -Llib -lmem

# Link (debug)
cc main.c -o app -Iinclude -DLIBMEM_DEBUG -Llib -lmem
```

```makefile
# Option B — use as a sub-project
LIBMEM = deps/libmem/libmem.a
$(LIBMEM):
	$(MAKE) -C deps/libmem all
```

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Versioning

This project follows [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

| Change | Bump |
|---|---|
| Bug fix, no API change | `PATCH` → `0.2.0 → 0.2.1` |
| New allocator function or type added, existing API unchanged | `MINOR` → `0.1.0 → 0.2.0` |
| Function signature changed, type removed, or struct layout broken | `MAJOR` → `0.x.y → 1.0.0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.  
> The API is not considered stable until `v1.0.0`.

### Current version

`v1.0.0` — defined in `include/libmem.h`:

```c
#define LIBMEM_VERSION_MAJOR 1
#define LIBMEM_VERSION_MINOR 0
#define LIBMEM_VERSION_PATCH 0
#define LIBMEM_VERSION       "1.0.0"
```

### Compile-time version check

```c
#include "libmem.h"

#if LIBMEM_VERSION_MAJOR >= 1
    /* Stable API: alignment, mark/rewind, usage queries, debug mode */
#endif
```

### Git tags

Each release is tagged in the repository:

```bash
git tag -a v1.0.0 -m "v1.0.0: stable API — alignment, mark/rewind, debug mode, usage queries, critical safety fixes"
git push origin v1.0.0
```

List all available tags: `git tag -l`
