#ifndef MEMORY_H
# define MEMORY_H

# include <stdio.h>
# include <stdint.h>

typedef struct {
    void *start_addr;
    void *free_head;
    size_t block_size;
    size_t total_blocks;
    size_t free_blocks;
} mem_pool_t;

void pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count);
void* pool_alloc(mem_pool_t *pool);
void pool_free(mem_pool_t *pool, void *ptr);

#endif