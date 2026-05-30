#include <stdint.h>
#include "libmem.h"

void pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count)
{
    uint8_t *curr;
    size_t   i;

    pool->start_addr   = buffer;
    pool->block_size   = block_size;
    pool->total_blocks = block_count;
    pool->free_blocks  = block_count;
    if (block_size < sizeof(void *))
    {
        pool->free_head = NULL;
        return ;
    }
    pool->free_head = buffer;
    curr = (uint8_t *)buffer;
    for (i = 0; i < block_count - 1; i++)
    {
        void **next_ptr_loc = (void **)curr;
        *next_ptr_loc = curr + block_size;
        curr += block_size;
    }
    *((void **)curr) = NULL;
}

void    *pool_alloc(mem_pool_t *pool)
{
    void    *allocated_block;

    if (pool->free_head == NULL)
        return (NULL);
    allocated_block = pool->free_head;
    pool->free_head = *((void **)allocated_block);
    pool->free_blocks--;
    return (allocated_block);
}

void    pool_free(mem_pool_t *pool, void *ptr)
{
    uint8_t *p;
    uint8_t *start;
    uint8_t *end;

    if (ptr == NULL)
        return ;
    p     = (uint8_t *)ptr;
    start = (uint8_t *)pool->start_addr;
    end   = start + pool->total_blocks * pool->block_size;
    if (p < start || p >= end)
        return ;
    if (pool->free_blocks >= pool->total_blocks)
        return ;
    *((void **)ptr) = pool->free_head;
    pool->free_head = ptr;
    pool->free_blocks++;
}

