/**
 * @file libmem.h
 * @brief libmem public API — pool, arena, and slab allocators.
 *
 * All allocators operate on caller-supplied buffers.  No internal malloc is
 * performed; the caller is fully responsible for backing storage lifetime.
 *
 * Allocator summary:
 *  - Pool  — fixed-size blocks, O(1) alloc/free via free-list.
 *  - Arena — bump allocator, O(1) alloc, O(1) reset (frees everything at once).
 *  - Slab  — variable-size backed by multiple pools, picks the smallest fitting pool.
 *
 * Debug mode
 * ----------
 * Compile with -DLIBMEM_DEBUG to enable:
 *  - Double-free detection in pool_free()  (O(n) free-list scan, stderr report).
 *  - Allocation statistics (alloc_count, peak_usage / peak_used) in each allocator.
 *  - arena_print_stats() and pool_print_stats() helpers.
 *
 * WARNING: struct sizes differ between debug and release builds.
 *          Never mix debug-compiled and release-compiled translation units.
 */

#ifndef LIBMEM_H
# define LIBMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Version ------------------------------------------------------------- */
# define LIBMEM_VERSION_MAJOR 0
# define LIBMEM_VERSION_MINOR 2
# define LIBMEM_VERSION_PATCH 0
# define LIBMEM_VERSION       "0.2.0"

# include <stddef.h>
# include <stdint.h>

/* ---- Alignment ----------------------------------------------------------- */

/**
 * @brief Default alignment for arena_alloc().
 *
 * Guaranteed to satisfy the alignment requirement of every scalar type on the
 * platform.  Derived from max_align_t (C11) when available; falls back to the
 * stricter of sizeof(void*) and sizeof(double) otherwise.
 */
# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define LIBMEM_MAX_ALIGN  (_Alignof(max_align_t))
# elif defined(__GNUC__) || defined(__clang__)
#  define LIBMEM_MAX_ALIGN  (__alignof__(double) >= __alignof__(void *) \
                              ? __alignof__(double) : __alignof__(void *))
# else
#  define LIBMEM_MAX_ALIGN  (8u)
# endif

/* ---- Pool ---------------------------------------------------------------- */

/**
 * @brief Fixed-size block pool allocator.
 *
 * Maintains a free-list of equal-size blocks inside a caller-supplied buffer.
 * Allocation and deallocation are both O(1).
 *
 * Constraints (checked in pool_init):
 *  - block_size  >= sizeof(void*) and a multiple of sizeof(void*).
 *  - buffer      aligned to at least sizeof(void*).
 *  - block_count > 0.
 *
 * If any constraint is violated pool_init() leaves free_head == NULL so every
 * subsequent pool_alloc() returns NULL safely.
 */
typedef struct {
    void   *start_addr;   /**< Start of the backing buffer.                  */
    void   *free_head;    /**< Head of the intrusive free-list.              */
    size_t  block_size;   /**< Size of each block in bytes.                  */
    size_t  total_blocks; /**< Total number of blocks in the pool.           */
    size_t  free_blocks;  /**< Number of currently free blocks.              */
#ifdef LIBMEM_DEBUG
    size_t  alloc_count;  /**< (debug) Cumulative allocations since init.    */
    size_t  peak_used;    /**< (debug) Peak number of in-use blocks.         */
#endif
} mem_pool_t;

/**
 * @brief Initialize a pool allocator.
 *
 * On constraint violation (block_count == 0, block_size < sizeof(void*),
 * misaligned block_size or buffer) the pool is left in a safe but unusable
 * state: free_head == NULL.  In debug builds a message is printed to stderr.
 *
 * @param pool        Uninitialized pool.
 * @param buffer      Caller-allocated backing storage.
 * @param block_size  Size of each block. Must be >= sizeof(void*) and a
 *                    multiple of sizeof(void*).
 * @param block_count Number of blocks available in @p buffer. Must be > 0.
 */
void    pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);

/**
 * @brief Allocate one block from the pool.
 * @param pool Initialized pool.
 * @return Pointer to a free block, or NULL if the pool is exhausted or was
 *         not initialized correctly.
 */
void   *pool_alloc(mem_pool_t *pool);

/**
 * @brief Return a block to the pool.
 *
 * Silently ignores NULL pointers and out-of-range pointers.
 * In debug builds, reports double-frees to stderr and returns without action.
 *
 * @param pool Owning pool (must be the pool the block was allocated from).
 * @param ptr  Block to free.
 */
void    pool_free(mem_pool_t *pool, void *ptr);

/**
 * @brief Return the number of blocks currently in use.
 * @param pool Initialized pool.
 */
size_t  pool_used(const mem_pool_t *pool);

/**
 * @brief Return the number of blocks still available for allocation.
 * @param pool Initialized pool.
 */
size_t  pool_remaining(const mem_pool_t *pool);

#ifdef LIBMEM_DEBUG
/**
 * @brief Print pool allocation statistics to stderr.
 * @note Only available when compiled with -DLIBMEM_DEBUG.
 */
void    pool_print_stats(const mem_pool_t *pool);
#endif

/* ---- Arena --------------------------------------------------------------- */

/**
 * @brief Opaque mark used for arena_rewind().
 *
 * Captures the current allocation cursor so that all subsequent allocations
 * can be discarded in O(1) with arena_rewind().
 *
 * @code
 *   mem_arena_mark_t mark = arena_mark(&arena);
 *   // ... temporary allocations ...
 *   arena_rewind(&arena, mark);   // everything after mark is reclaimed
 * @endcode
 */
typedef size_t mem_arena_mark_t;

/**
 * @brief Linear (bump) allocator.
 *
 * Allocates sequentially from a caller-supplied buffer.  Individual frees are
 * not supported; use arena_reset() to reclaim everything at once, or
 * arena_mark() / arena_rewind() for partial reclamation.
 *
 * arena_alloc() always returns memory aligned to LIBMEM_MAX_ALIGN bytes.
 * Use arena_alloc_aligned() for a custom alignment.
 */
typedef struct {
    void   *start_addr; /**< Start of the backing buffer.           */
    size_t  offset;     /**< Current allocation cursor (bytes).     */
    size_t  size;       /**< Total capacity of the buffer in bytes. */
#ifdef LIBMEM_DEBUG
    size_t  alloc_count; /**< (debug) Allocations since last reset. */
    size_t  peak_usage;  /**< (debug) Peak offset since init.       */
#endif
} mem_arena_t;

/**
 * @brief Initialize an arena allocator.
 * @param arena  Uninitialized arena.
 * @param buffer Caller-allocated backing storage.
 * @param size   Capacity of @p buffer in bytes.
 */
void    arena_init(mem_arena_t *arena, void *buffer, size_t size);

/**
 * @brief Bump-allocate @p alloc_size bytes aligned to LIBMEM_MAX_ALIGN.
 *
 * Padding bytes may be inserted before the returned pointer to satisfy
 * alignment, so the consumed arena space can be larger than @p alloc_size.
 *
 * @param arena      Initialized arena.
 * @param alloc_size Number of bytes to allocate (must be > 0).
 * @return Pointer to the allocated region, or NULL on failure (zero size,
 *         insufficient space, or integer overflow in offset arithmetic).
 */
void   *arena_alloc(mem_arena_t *arena, size_t alloc_size);

/**
 * @brief Bump-allocate with an explicit alignment.
 *
 * @param arena      Initialized arena.
 * @param alloc_size Number of bytes to allocate (must be > 0).
 * @param align      Required alignment in bytes.  Must be a power of 2 and
 *                   greater than zero.
 * @return Pointer to the allocated region, or NULL on failure.
 */
void   *arena_alloc_aligned(mem_arena_t *arena, size_t alloc_size, size_t align);

/**
 * @brief Free all arena allocations at once by resetting the cursor to zero.
 *
 * In debug builds, alloc_count is reset to 0 while peak_usage is preserved
 * (it reflects the historical high-water mark since arena_init()).
 *
 * @param arena Initialized arena.
 */
void    arena_reset(mem_arena_t *arena);

/**
 * @brief Save the current allocation cursor.
 * @param arena Initialized arena.
 * @return An opaque mark representing the current cursor position.
 */
mem_arena_mark_t  arena_mark(const mem_arena_t *arena);

/**
 * @brief Restore the allocation cursor to a previously saved mark.
 *
 * All memory allocated after @p mark was taken is effectively freed.
 * @p mark must have been obtained from the same arena and must not be
 * greater than the current offset (i.e., you cannot rewind forward).
 *
 * @param arena Initialized arena.
 * @param mark  Mark previously returned by arena_mark().
 */
void    arena_rewind(mem_arena_t *arena, mem_arena_mark_t mark);

/**
 * @brief Return the number of bytes consumed by allocations (including padding).
 * @param arena Initialized arena.
 */
size_t  arena_used(const mem_arena_t *arena);

/**
 * @brief Return the number of bytes still available for allocation.
 * @param arena Initialized arena.
 */
size_t  arena_remaining(const mem_arena_t *arena);

#ifdef LIBMEM_DEBUG
/**
 * @brief Print arena allocation statistics to stderr.
 * @note Only available when compiled with -DLIBMEM_DEBUG.
 */
void    arena_print_stats(const mem_arena_t *arena);
#endif

/* ---- Slab ---------------------------------------------------------------- */

/** @brief Maximum number of pools a slab allocator can manage. */
# define SLAB_MAX_POOLS 8

/**
 * @brief Configuration for a single pool inside a slab allocator.
 */
typedef struct {
    void   *buffer;      /**< Backing buffer for this pool.              */
    size_t  block_size;  /**< Fixed block size of this pool in bytes.    */
    size_t  block_count; /**< Number of blocks available in the pool.    */
} mem_slab_config_t;

/**
 * @brief Multi-pool slab allocator for variable-size allocations.
 *
 * Contains up to SLAB_MAX_POOLS pools ordered by block size. slab_alloc()
 * selects the smallest pool whose block size fits the requested size.
 */
typedef struct {
    mem_pool_t  pools[SLAB_MAX_POOLS]; /**< Array of fixed-size pools.         */
    size_t      count;                 /**< Number of active pools.            */
} mem_slab_t;

/**
 * @brief Initialize a slab allocator from an array of pool configurations.
 *
 * Pools are sorted internally by block_size so that slab_alloc() can perform
 * a linear first-fit scan.  count is silently clamped to SLAB_MAX_POOLS.
 *
 * @param slab    Uninitialized slab.
 * @param configs Array of pool configurations.
 * @param count   Number of entries in @p configs. Must be <= SLAB_MAX_POOLS.
 */
void    slab_init(mem_slab_t *slab, mem_slab_config_t *configs, size_t count);

/**
 * @brief Allocate from the smallest pool that can satisfy the request.
 * @param slab Initialized slab.
 * @param size Requested allocation size in bytes.
 * @return Pointer to an allocated block, or NULL if no pool can satisfy the request.
 */
void   *slab_alloc(mem_slab_t *slab, size_t size);

/**
 * @brief Return an allocation back to the appropriate pool.
 *
 * The slab determines the correct pool by matching the pointer range.
 * Silently ignores NULL pointers and out-of-range pointers.
 *
 * @param slab Owning slab.
 * @param ptr  Pointer previously returned by slab_alloc().
 */
void    slab_free(mem_slab_t *slab, void *ptr);

#ifdef __cplusplus
}
#endif
#endif /* LIBMEM_H */