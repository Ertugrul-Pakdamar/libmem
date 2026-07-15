#include <stddef.h>
#include <stdint.h>
#include "libmem.h"

#ifdef LIBMEM_DEBUG
# include <stdio.h>
#endif

/* ---- Internal helpers ---------------------------------------------------- */

/**
 * Round @p x up to the nearest multiple of @p align.
 * @p align must be a non-zero power of 2.
 */
static size_t align_up(size_t x, size_t align)
{
    return (x + align - 1u) & ~(align - 1u);
}

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
    size_t  aligned_offset;
    void   *ptr;

    if (alloc_size == 0 || align == 0)
        return (NULL);

    aligned_offset = align_up(arena->offset, align);

    /*
     * Overflow-safe capacity check.
     * Equivalent to: aligned_offset + alloc_size > arena->size
     * but written as two separate comparisons to avoid wrap-around on
     * SIZE_MAX-sized values.
     */
    if (aligned_offset > arena->size
        || alloc_size > arena->size - aligned_offset)
        return (NULL);

    ptr           = (uint8_t *)arena->start_addr + aligned_offset;
    arena->offset = aligned_offset + alloc_size;

#ifdef LIBMEM_DEBUG
    arena->alloc_count++;
    if (arena->offset > arena->peak_usage)
        arena->peak_usage = arena->offset;
#endif

    return (ptr);
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
