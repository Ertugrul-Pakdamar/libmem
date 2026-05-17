#include "memory.h"

void pool_init(mem_pool_t *pool, void *buffer, size_t block_size, size_t block_count)
{
    pool->start_addr = buffer;
    pool->block_size = block_size;
    pool->total_blocks = block_count;
    pool->free_blocks = block_count;
    
    pool->free_head = buffer;
    
    uint8_t *curr = (uint8_t *)buffer;
    for (size_t i = 0; i < block_count - 1; i++)
    {
        void **next_ptr_loc = (void **)curr; 
        *next_ptr_loc = curr + block_size;
        curr += block_size;
    }
    *((void **)curr) = NULL; 
}

void* pool_alloc(mem_pool_t *pool)
{
    if (pool->free_head == NULL)
        return NULL;
    
    void *allocated_block = pool->free_head;
    
    pool->free_head = *((void **)allocated_block);
    pool->free_blocks--;
    
    return allocated_block;
}

void pool_free(mem_pool_t *pool, void *ptr)
{
    if (ptr == NULL)
        return;
    
    *((void **)ptr) = pool->free_head;
    
    pool->free_head = ptr;
    pool->free_blocks++;
}
