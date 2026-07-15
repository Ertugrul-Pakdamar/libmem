# Contributing to libmem

Contributions are welcome. Please read this guide before opening a PR.

---

## What can be contributed

- Bug fixes in any allocator (`src/pool.c`, `src/arena.c`, `src/slab.c`)
- New allocator strategies (e.g. a buddy allocator, a stack allocator)
- New examples in `examples/` (see [Adding an example](#adding-an-example) below)
- Documentation improvements and usage examples

---

## How to contribute

```bash
# 1. Fork the repo on GitHub
# 2. Clone your fork
git clone https://github.com/Ertugrul-Pakdamar/libmem.git
cd libmem

# 3. Create a feature branch
git checkout -b feature/my-improvement

# 4. Make changes, verify both builds are clean
make re
make debug

# 5. Open a pull request against main
```

---

## Code style

- **Standard:** C11, compiled with `-Wall -Wextra -Werror` — zero warnings required
  in both release and debug builds (`make re` and `make debug`).
- **No dynamic allocation:** `malloc` / `free` are forbidden inside the library.
  All memory is caller-supplied.
- **Public API:** every new public function must be declared in `include/libmem.h`
  with a Doxygen `/** @brief @param @return */` comment.
- **Naming:** `snake_case` throughout; types use the `mem_` prefix (e.g. `mem_pool_t`).
- **Debug guards:** code that is only active under `-DLIBMEM_DEBUG` must be wrapped
  in `#ifdef LIBMEM_DEBUG … #endif`. Debug-only struct fields go **at the end**
  of the struct to minimise layout differences.
- **Alignment:** all arena allocations must respect `LIBMEM_MAX_ALIGN`. Pool
  buffers and block sizes must be multiples of `sizeof(void*)`.

---

## Debug mode

When adding or modifying allocator logic, verify that it also compiles and
behaves correctly under `-DLIBMEM_DEBUG`:

```bash
make debug
```

The debug build enables double-free detection, allocation statistics, and
diagnostic messages to `stderr`. See the **Debug Mode** section in
[README.md](README.md) for details.

---

## Adding an example

Examples live in `examples/` and are built automatically by `make examples`.
Each example is a single `.c` file that links only against `libmem.a`.

### Naming convention

```
examples/NN_short_name.c     (NN = two-digit number, e.g. 04_arena_mark.c)
```

### Checklist

1. Compiles without warnings under `-Wall -Wextra -Werror`.
2. Top-of-file doc comment explains what the example demonstrates, how to build
   it (`make examples`), and how to run it (`./examples/bin/NN_name`).
3. `main()` returns `0` on success, non-zero on failure.
4. Uses only stack/static backing buffers — no `malloc` / `free`.
5. Add a row to the **Examples** table in `README.md`.

---

## Bumping the Version

When a contribution changes the public API, the version macros in
`include/libmem.h` must be updated as part of the **same commit**.  
Follow [Semantic Versioning](https://semver.org/):

| What changed | Which macro | Reset |
|---|---|---|
| Bug fix, no API change | `PATCH` | — |
| New public function or type added, existing API unchanged | `MINOR` | `PATCH → 0` |
| Existing function removed or signature changed | `MAJOR` | `MINOR → 0`, `PATCH → 0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.

### Steps

1. Update the three numeric macros in `include/libmem.h`.
2. Update the `LIBMEM_VERSION` string to match.
3. The maintainer will create a git tag after merging (`git tag vX.Y.Z`).
