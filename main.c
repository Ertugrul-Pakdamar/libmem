#include "memory.h"

#define BLOCK_SIZE 32
#define POOL_SIZE 100

int main()
{
    uint8_t buffer[BLOCK_SIZE * POOL_SIZE];
    mem_pool_t mem_pool;

    pool_init(&mem_pool, buffer, BLOCK_SIZE, POOL_SIZE);

    char *str = pool_alloc(&mem_pool);
    for (int i = 0; i < 31; i++)
        str[i] = 'a';
    str[31] = 0;
    printf("%s\n", str);

    pool_free(&mem_pool, str);

    return (0);
}