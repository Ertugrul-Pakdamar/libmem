#include <stddef.h>
#include <stdint.h>
#include "libmem.h"

#ifdef LIBMEM_DEBUG
# include <stdio.h>

/*
 * Scan the free-list linearly to detect whether @p ptr is already freed.
 * O(n) — only used in debug builds.
 */
static int pool_ptr_in_freelist(const mem_pool_t *pool, const void *ptr)
{
    const void *curr;

    curr = pool->free_head;
    while (curr != NULL)
    {
        if (curr == ptr)
            return (1);
        curr = *((const void * const *)curr);
    }
    return (0);
}
#endif /* LIBMEM_DEBUG */

/* ---- pool_init ----------------------------------------------------------- */

void    pool_init(mem_pool_t *pool, void *buffer,
                  size_t block_size, size_t block_count)
{
    uint8_t *curr;
    size_t   i;
    size_t   min_align;

    /*
     * Initialize all fields to a safe "unusable" state first.
     * total_blocks and free_blocks are set to 0 here so that if any
     * validation below fails, slab_alloc() will correctly skip this pool
     * (free_blocks == 0 prevents it from being selected) and pool_free()
     * will safely reject any pointer via the saturation guard.
     */
    pool->start_addr   = buffer;
    pool->block_size   = block_size;
    pool->total_blocks = 0;
    pool->free_blocks  = 0;
    pool->free_head    = NULL;

#ifdef LIBMEM_DEBUG
    pool->alloc_count = 0;
    pool->peak_used   = 0;
#endif

    min_align = sizeof(void *);

    /* --- Validate block_count -------------------------------------------- */
    if (block_count == 0)
    {
#ifdef LIBMEM_DEBUG
        fprintf(stderr,
            "[libmem/pool] pool_init: block_count == 0 — pool unusable.\n");
#endif
        return ;
    }

    /* --- Validate block_size --------------------------------------------- */
    if (block_size < min_align)
    {
#ifdef LIBMEM_DEBUG
        fprintf(stderr,
            "[libmem/pool] pool_init: block_size (%zu) < sizeof(void*) (%zu)"
            " — pool unusable.\n", block_size, min_align);
#endif
        return ;
    }

    if (block_size % min_align != 0)
    {
#ifdef LIBMEM_DEBUG
        fprintf(stderr,
            "[libmem/pool] pool_init: block_size (%zu) is not a multiple of"
            " sizeof(void*) (%zu) — pool unusable.\n", block_size, min_align);
#endif
        return ;
    }

    /* --- Validate buffer alignment ---------------------------------------- */
    if (((uintptr_t)buffer % min_align) != 0)
    {
#ifdef LIBMEM_DEBUG
        fprintf(stderr,
            "[libmem/pool] pool_init: buffer (%p) is not aligned to"
            " sizeof(void*) (%zu) — pool unusable.\n", buffer, min_align);
#endif
        return ;
    }

    /* --- All validation passed: build the free-list ----------------------- */
    pool->total_blocks = block_count;
    pool->free_blocks  = block_count;
    pool->free_head    = buffer;

    curr = (uint8_t *)buffer;
    for (i = 0; i < block_count - 1; i++)
    {
        void **next_ptr_loc = (void **)curr;
        *next_ptr_loc = curr + block_size;
        curr += block_size;
    }
    *((void **)curr) = NULL;
}

/* ---- pool_alloc ---------------------------------------------------------- */

void    *pool_alloc(mem_pool_t *pool)
{
    void    *allocated_block;

    if (pool->free_head == NULL)
        return (NULL);
    allocated_block  = pool->free_head;
    pool->free_head  = *((void **)allocated_block);
    pool->free_blocks--;

#ifdef LIBMEM_DEBUG
    pool->alloc_count++;
    {
        size_t used = pool->total_blocks - pool->free_blocks;
        if (used > pool->peak_used)
            pool->peak_used = used;
    }
#endif

    return (allocated_block);
}

/* ---- pool_free ----------------------------------------------------------- */

void    pool_free(mem_pool_t *pool, void *ptr)
{
    uint8_t *p;
    uint8_t *start;
    uint8_t *end;
    size_t   block_offset;

    if (ptr == NULL)
        return ;

    /*
     * Guard against an invalidly initialized pool (block_size == 0 would
     * cause a division-by-zero in the block-boundary check below).
     */
    if (pool->block_size == 0 || pool->total_blocks == 0)
        return ;

    p     = (uint8_t *)ptr;
    start = (uint8_t *)pool->start_addr;
    end   = start + pool->total_blocks * pool->block_size;

    /* Bounds check: reject pointers outside this pool's buffer. */
    if (p < start || p >= end)
        return ;

    /*
     * Block-boundary check: reject pointers that do not point to the start
     * of a block.  Without this, a caller passing (block + 1) would corrupt
     * the free-list by writing a void* into an unaligned location.
     *
     * This also protects slab_free() indirectly, since slab_free() delegates
     * to pool_free() after a range match.
     */
    block_offset = (size_t)(p - start);
    if (block_offset % pool->block_size != 0)
        return ;

    /* Saturation guard: do not increment free_blocks past total_blocks. */
    if (pool->free_blocks >= pool->total_blocks)
        return ;

#ifdef LIBMEM_DEBUG
    /* Double-free detection: O(n) scan of the free-list. */
    if (pool_ptr_in_freelist(pool, ptr))
    {
        fprintf(stderr,
            "[libmem/pool] double-free detected: ptr=%p pool=%p\n",
            ptr, (void *)pool);
        return ;
    }
#endif

    *((void **)ptr) = pool->free_head;
    pool->free_head  = ptr;
    pool->free_blocks++;
}

/* ---- pool_used / pool_remaining ----------------------------------------- */

size_t  pool_used(const mem_pool_t *pool)
{
    return (pool->total_blocks - pool->free_blocks);
}

size_t  pool_remaining(const mem_pool_t *pool)
{
    return (pool->free_blocks);
}

/* ---- pool_print_stats (debug only) -------------------------------------- */

#ifdef LIBMEM_DEBUG
void    pool_print_stats(const mem_pool_t *pool)
{
    fprintf(stderr,
        "[libmem/pool] total=%zu  used=%zu  free=%zu"
        "  alloc_count=%zu  peak_used=%zu\n",
        pool->total_blocks,
        pool_used(pool),
        pool->free_blocks,
        pool->alloc_count,
        pool->peak_used);
}
#endif
