/**
 * @file libmem.h
 * @brief libmem public API — pool, arena, and slab allocators.
 *
 * All allocators operate on caller-supplied buffers. No internal malloc is
 * performed; the caller is fully responsible for backing storage lifetime.
 *
 * Allocator summary:
 *  - Pool  — fixed-size blocks, O(1) alloc/free via free-list.
 *  - Arena — bump allocator, O(1) alloc, O(1) reset (frees everything at once).
 *  - Slab  — variable-size backed by multiple pools, picks the smallest fitting pool.
 */

#ifndef LIBMEM_H
# define LIBMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Version ------------------------------------------------------------- */
# define LIBMEM_VERSION_MAJOR 0
# define LIBMEM_VERSION_MINOR 1
# define LIBMEM_VERSION_PATCH 0
# define LIBMEM_VERSION       "0.1.0"

# include <stddef.h>
# include <stdint.h>

/* ---- Pool ---------------------------------------------------------------- */

/**
 * @brief Fixed-size block pool allocator.
 *
 * Maintains a free-list of equal-size blocks inside a caller-supplied buffer.
 * Allocation and deallocation are both O(1).
 */
typedef struct {
    void   *start_addr;   /**< Start of the backing buffer.                  */
    void   *free_head;    /**< Head of the intrusive free-list.              */
    size_t  block_size;   /**< Size of each block in bytes.                  */
    size_t  total_blocks; /**< Total number of blocks in the pool.           */
    size_t  free_blocks;  /**< Number of currently free blocks.              */
} mem_pool_t;

/**
 * @brief Initialize a pool allocator.
 * @param pool        Uninitialized pool.
 * @param buffer      Caller-allocated backing storage.
 * @param block_size  Size of each block. Must be >= sizeof(void*).
 * @param block_count Number of blocks available in @p buffer.
 */
void    pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);

/**
 * @brief Allocate one block from the pool.
 * @param pool Initialized pool.
 * @return Pointer to a free block, or NULL if the pool is exhausted.
 */
void   *pool_alloc(mem_pool_t *pool);

/**
 * @brief Return a block to the pool.
 * @param pool Owning pool (must be the pool the block was allocated from).
 * @param ptr  Block to free. Must not be NULL.
 */
void    pool_free(mem_pool_t *pool, void *ptr);

/* ---- Arena --------------------------------------------------------------- */

/**
 * @brief Linear (bump) allocator.
 *
 * Allocates sequentially from a caller-supplied buffer. Individual frees are
 * not supported; reset the entire arena with arena_reset().
 */
typedef struct {
    void   *start_addr; /**< Start of the backing buffer.           */
    size_t  offset;     /**< Current allocation cursor (bytes).     */
    size_t  size;       /**< Total capacity of the buffer in bytes. */
} mem_arena_t;

/**
 * @brief Initialize an arena allocator.
 * @param arena  Uninitialized arena.
 * @param buffer Caller-allocated backing storage.
 * @param size   Capacity of @p buffer in bytes.
 */
void    arena_init(mem_arena_t *arena, void *buffer, size_t size);

/**
 * @brief Bump-allocate from the arena.
 * @param arena      Initialized arena.
 * @param alloc_size Number of bytes to allocate.
 * @return Pointer to the allocated region, or NULL if there is insufficient space.
 */
void   *arena_alloc(mem_arena_t *arena, size_t alloc_size);

/**
 * @brief Free all arena allocations at once by resetting the cursor to zero.
 * @param arena Initialized arena.
 */
void    arena_reset(mem_arena_t *arena);

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
 * The slab determines the correct pool by matching the block size.
 * @param slab Owning slab.
 * @param ptr  Pointer previously returned by slab_alloc().
 */
void    slab_free(mem_slab_t *slab, void *ptr);

#ifdef __cplusplus
}
#endif
#endif