# Contributing to libmem

Contributions are welcome. Please read this guide before opening a PR.

---

## What can be contributed

- Bug fixes in any allocator (`src/pool.c`, `src/arena.c`, `src/slab.c`)
- New allocator strategies (e.g. a buddy allocator, a stack allocator)
- Alignment support improvements
- Additional introspection helpers (e.g. `pool_capacity`, `arena_remaining`)
- New examples in `examples/` (see [Adding an example](#adding-an-example) below)
- Documentation and usage examples

---

## How to contribute

```bash
# 1. Fork the repo on GitHub
# 2. Clone your fork
git clone https://github.com/<your-username>/libmem.git
cd libmem

# 3. Create a feature branch
git checkout -b feature/my-improvement

# 4. Make changes, verify the build is clean
make re

# 5. Open a pull request against main
```

---

## Code style

- C11, compiled with `-Wall -Wextra -Werror` — zero warnings required
- No `malloc` / `free` inside the library — all memory is caller-supplied
- New public functions must be declared in `include/memory.h` with Doxygen
  `/** @brief @param @return */` comments
- Follow the existing `snake_case` naming convention and `mem_` prefix for types

---

## Adding an example

Examples live in `examples/` and are built with `make examples`. Each example
is a single `.c` file that links only against `libmem.a` — no other libraries.

### Naming convention

```
examples/NN_short_name.c     (NN = two-digit number, e.g. 04_pool_introspect.c)
```

### Checklist

1. Compiles without warnings under `-Wall -Wextra -Werror`.
2. Top-of-file doc comment explains what the example demonstrates, how to build,
   and how to run it.
3. `main()` returns `0` on success, non-zero on failure.
4. Uses only stack/static backing buffers — no `malloc` / `free`.
5. Add a row to the **Examples** table in `README.md`.

---

## Bumping the Version

When a contribution changes the public API, the version macros in `include/memory.h`
must be updated as part of the **same commit**.  
Follow [Semantic Versioning](https://semver.org/):

| What changed | Which macro | Reset |
|---|---|---|
| Bug fix, no API change | `PATCH` | — |
| New allocator function or type added | `MINOR` | `PATCH → 0` |
| Existing function removed or signature changed | `MAJOR` | `MINOR → 0`, `PATCH → 0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.

### Steps

1. Update the three numeric macros in `include/memory.h`.
2. Update `LIBMEM_VERSION` string to match.
3. The maintainer will create a git tag after merging (`git tag vX.Y.Z`).
