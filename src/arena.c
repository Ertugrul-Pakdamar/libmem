#include <stddef.h>
#include <stdint.h>
#include "libmem.h"

#ifdef LIBMEM_DEBUG
# include <stdio.h>
#endif

/* ---- arena_init ---------------------------------------------------------- */

void    arena_init(mem_arena_t *arena, void *buffer, size_t size)
{
    arena->start_addr = buffer;
    arena->offset     = 0;
    arena->size       = size;
#ifdef LIBMEM_DEBUG
    arena->alloc_count = 0;
    arena->peak_usage  = 0;
#endif
}

/* ---- arena_alloc_aligned ------------------------------------------------- */

void    *arena_alloc_aligned(mem_arena_t *arena, size_t alloc_size, size_t align)
{
    uintptr_t current;
    uintptr_t aligned_addr;
    size_t    padding;
    size_t    aligned_offset;

    /* --- Parameter validation -------------------------------------------- */
    if (alloc_size == 0 || align == 0)
        return (NULL);

    /*
     * align must be a power of 2.
     * align=3, 6, 12 etc. would produce incorrect results in bitmask
     * arithmetic; reject them explicitly.
     */
    if ((align & (align - 1u)) != 0)
        return (NULL);

    /*
     * Align the *real* pointer address, not just the offset.
     * If arena->start_addr is itself misaligned (e.g. 0x1003), aligning
     * only the offset would leave the returned pointer misaligned.
     */
    current = (uintptr_t)arena->start_addr + (uintptr_t)arena->offset;

    /*
     * Overflow-safe alignment arithmetic on the actual address.
     * Without this guard, `current + (align - 1)` could wrap around on
     * platforms where uintptr_t == SIZE_MAX.
     */
    if (align > 1u && current > (uintptr_t)(UINTPTR_MAX - (align - 1u)))
        return (NULL);

    aligned_addr = (current + (uintptr_t)(align - 1u))
                   & ~(uintptr_t)(align - 1u);

    /* Bytes of padding inserted to reach the alignment boundary. */
    padding = (size_t)(aligned_addr - current);

    /*
     * Overflow-safe capacity checks.
     *   1) padding fits in remaining space (guards aligned_offset arithmetic)
     *   2) alloc_size fits in the space after padding
     */
    if (padding > arena->size - arena->offset)
        return (NULL);
    aligned_offset = arena->offset + padding;
    if (alloc_size > arena->size - aligned_offset)
        return (NULL);

    arena->offset = aligned_offset + alloc_size;

#ifdef LIBMEM_DEBUG
    arena->alloc_count++;
    if (arena->offset > arena->peak_usage)
        arena->peak_usage = arena->offset;
#endif

    return ((void *)aligned_addr);
}

/* ---- arena_alloc --------------------------------------------------------- */

void    *arena_alloc(mem_arena_t *arena, size_t alloc_size)
{
    return (arena_alloc_aligned(arena, alloc_size, LIBMEM_MAX_ALIGN));
}

/* ---- arena_reset --------------------------------------------------------- */

void    arena_reset(mem_arena_t *arena)
{
    arena->offset = 0;
#ifdef LIBMEM_DEBUG
    /* alloc_count reflects allocations since the last reset. */
    arena->alloc_count = 0;
    /* peak_usage is NOT reset — it is a cumulative high-water mark. */
#endif
}

/* ---- arena_mark / arena_rewind ------------------------------------------ */

mem_arena_mark_t    arena_mark(const mem_arena_t *arena)
{
    return (arena->offset);
}

void    arena_rewind(mem_arena_t *arena, mem_arena_mark_t mark)
{
    /*
     * Silently ignore a forward rewind (mark > current offset) — doing so
     * would corrupt the arena by exposing uninitialized memory.
     */
    if (mark <= arena->offset)
        arena->offset = mark;
}

/* ---- arena_used / arena_remaining --------------------------------------- */

size_t  arena_used(const mem_arena_t *arena)
{
    return (arena->offset);
}

size_t  arena_remaining(const mem_arena_t *arena)
{
    return (arena->size - arena->offset);
}

/* ---- arena_print_stats (debug only) ------------------------------------- */

#ifdef LIBMEM_DEBUG
void    arena_print_stats(const mem_arena_t *arena)
{
    fprintf(stderr,
        "[libmem/arena] capacity=%zu  used=%zu  remaining=%zu"
        "  alloc_count=%zu  peak_usage=%zu\n",
        arena->size,
        arena->offset,
        arena->size - arena->offset,
        arena->alloc_count,
        arena->peak_usage);
}
#endif
