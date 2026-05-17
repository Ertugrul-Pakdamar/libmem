#include "memory.h"

static void sort_pools(mem_pool_t *pools, size_t count)
{
    size_t      i;
    size_t      j;
    mem_pool_t  tmp;

    for (i = 1; i < count; i++)
    {
        tmp = pools[i];
        j = i;
        while (j > 0 && pools[j - 1].block_size > tmp.block_size)
        {
            pools[j] = pools[j - 1];
            j--;
        }
        pools[j] = tmp;
    }
}

void    slab_init(mem_slab_t *slab, mem_slab_config_t *configs, size_t count)
{
    size_t  i;

    if (count > SLAB_MAX_POOLS)
        count = SLAB_MAX_POOLS;
    slab->count = count;
    for (i = 0; i < count; i++)
        pool_init(&slab->pools[i], configs[i].buffer,
            configs[i].block_size, configs[i].block_count);
    sort_pools(slab->pools, count);
}

void    *slab_alloc(mem_slab_t *slab, size_t size)
{
    size_t  i;

    for (i = 0; i < slab->count; i++)
    {
        if (slab->pools[i].block_size >= size
            && slab->pools[i].free_blocks > 0)
            return (pool_alloc(&slab->pools[i]));
    }
    return (NULL);
}

void    slab_free(mem_slab_t *slab, void *ptr)
{
    size_t   i;
    uint8_t *p;
    uint8_t *start;
    uint8_t *end;

    if (ptr == NULL)
        return ;
    p = (uint8_t *)ptr;
    for (i = 0; i < slab->count; i++)
    {
        start = (uint8_t *)slab->pools[i].start_addr;
        end   = start + slab->pools[i].total_blocks * slab->pools[i].block_size;
        if (p >= start && p < end)
        {
            pool_free(&slab->pools[i], ptr);
            return ;
        }
    }
}
