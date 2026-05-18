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
| Arena | O(1) | O(1) reset-only | Short-lived scratch memory, frame allocations |
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
│   └── memory.h             public API  ← start here
├── examples/
│   ├── 01_pool.c            pool alloc / free / reuse
│   ├── 02_arena.c           bump allocation, reset, reuse
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
make all        Build libmem.a
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
| `examples/bin/02_arena` | `examples/02_arena.c` | Bump allocation, `arena_reset`, second-round reuse |
| `examples/bin/03_slab` | `examples/03_slab.c` | Multi-pool slab, automatic pool selection, over-size rejection |

---

## Pool Allocator

Maintains an intrusive free-list inside a caller-supplied buffer. Every block
is the same size. Both alloc and free are O(1).

```c
#include "memory.h"

static unsigned char buf[sizeof(MyStruct) * 32];

mem_pool_t pool;
pool_init(&pool, buf, sizeof(MyStruct), 32);

MyStruct *obj = pool_alloc(&pool);  /* NULL if pool is exhausted */
/* ... use obj ... */
pool_free(&pool, obj);
```

---

## Arena Allocator

Bump allocator: advances a cursor on every allocation. Individual frees are
not supported — reset everything at once with `arena_reset()`.

```c
static unsigned char buf[4096];

mem_arena_t arena;
arena_init(&arena, buf, sizeof(buf));

void *a = arena_alloc(&arena, 64);   /* NULL if out of space */
void *b = arena_alloc(&arena, 128);

arena_reset(&arena);                 /* releases everything at once */
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
/* Initialize a pool allocator over a caller-supplied buffer. */
void  pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);

/* Allocate one block. Returns NULL if exhausted. */
void *pool_alloc(mem_pool_t *pool);

/* Return a block to the pool. ptr must not be NULL. */
void  pool_free(mem_pool_t *pool, void *ptr);
```

### Arena

```c
/* Initialize a bump allocator over a caller-supplied buffer. */
void  arena_init(mem_arena_t *arena, void *buffer, size_t size);

/* Allocate alloc_size bytes. Returns NULL if out of space. */
void *arena_alloc(mem_arena_t *arena, size_t alloc_size);

/* Release all arena allocations by resetting the cursor. */
void  arena_reset(mem_arena_t *arena);
```

### Slab

```c
/* Initialize a slab from an array of pool configurations. */
void  slab_init(mem_slab_t *slab, mem_slab_config_t *configs, size_t count);

/* Allocate from the smallest pool that fits size. Returns NULL on failure. */
void *slab_alloc(mem_slab_t *slab, size_t size);

/* Return an allocation to its owning pool. */
void  slab_free(mem_slab_t *slab, void *ptr);
```

---

## Integration into Your Project

```bash
# Option A — copy files
cp libmem.a          /your/project/lib/
cp include/memory.h  /your/project/include/

# Link
cc main.c -o app -Iinclude -Llib -lmem
```

```makefile
# Option B — use as a sub-project
LIBMEM = deps/mem/libmem.a
$(LIBMEM):
	$(MAKE) -C deps/mem all
```

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Versioning

This project follows [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

| Change | Bump |
|---|---|
| Bug fix, no API change | `PATCH` → `0.1.0 → 0.1.1` |
| New allocator function or type added, existing API unchanged | `MINOR` → `0.1.0 → 0.2.0` |
| Function signature changed, type removed, or struct layout broken | `MAJOR` → `0.x.y → 1.0.0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.  
> The API is not considered stable until `v1.0.0`.

### Current version

`v0.1.0` — defined in `include/memory.h`:

```c
#define LIBMEM_VERSION_MAJOR 0
#define LIBMEM_VERSION_MINOR 1
#define LIBMEM_VERSION_PATCH 0
#define LIBMEM_VERSION       "0.1.0"
```

### Compile-time version check

```c
#include "include/memory.h"

#if LIBMEM_VERSION_MAJOR == 0 && LIBMEM_VERSION_MINOR >= 1
    /* use a feature added in v0.1 */
#endif
```

### Git tags

Each release is tagged in the repository:

```bash
git tag -a v0.1.0 -m "Initial public release"
git push origin v0.1.0
```

List all available tags: `git tag -l`
