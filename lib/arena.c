#include "memory.h"

void    arena_init(mem_arena_t *arena, void *buffer, size_t size)
{
    arena->start_addr = buffer;
    arena->offset     = 0;
    arena->size       = size;
}

void    *arena_alloc(mem_arena_t *arena, size_t alloc_size)
{
    void    *ptr;

    if (alloc_size == 0 || arena->offset + alloc_size > arena->size)
        return (NULL);
    ptr            = (uint8_t *)arena->start_addr + arena->offset;
    arena->offset += alloc_size;
    return (ptr);
}

void    arena_reset(mem_arena_t *arena)
{
    arena->offset = 0;
}
