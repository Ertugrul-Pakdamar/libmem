#ifndef MEMORY_H
# define MEMORY_H

# include <stdio.h>
# include <stdint.h>
# include <stddef.h>

typedef struct {
    void   *start_addr;
    void   *free_head;
    size_t  block_size;
    size_t  total_blocks;
    size_t  free_blocks;
} mem_pool_t;

typedef struct {
    void   *start_addr;
    size_t  offset;
    size_t  size;
} mem_arena_t;

/* Pool */
void    pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);
void   *pool_alloc(mem_pool_t *pool);
void    pool_free(mem_pool_t *pool, void *ptr);

/* Arena */
void    arena_init(mem_arena_t *arena, void *buffer, size_t size);
void   *arena_alloc(mem_arena_t *arena, size_t alloc_size);
void    arena_reset(mem_arena_t *arena);

# define SLAB_MAX_POOLS 8

typedef struct {
    void   *buffer;
    size_t  block_size;
    size_t  block_count;
} mem_slab_config_t;

typedef struct {
    mem_pool_t  pools[SLAB_MAX_POOLS];
    size_t      count;
} mem_slab_t;

/* Slab */
void    slab_init(mem_slab_t *slab, mem_slab_config_t *configs, size_t count);
void   *slab_alloc(mem_slab_t *slab, size_t size);
void    slab_free(mem_slab_t *slab, void *ptr);

#endif