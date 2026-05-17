/**
 * examples/01_pool.c
 *
 * Demonstrates the pool allocator:
 *   - Backing buffer lives on the stack (or static storage).
 *   - pool_alloc / pool_free are O(1) via a free-list.
 *   - Exhaustion returns NULL; freed blocks are immediately reusable.
 *
 * Build:  make examples
 * Run:    ./examples/bin/01_pool
 */

#include <stdio.h>
#include <string.h>
#include "memory.h"

#define BLOCK_SIZE  32
#define BLOCK_COUNT  4

typedef struct { char text[BLOCK_SIZE]; } t_slot;

int main(void)
{
    /* Static backing buffer — no malloc anywhere. */
    static unsigned char buf[BLOCK_SIZE * BLOCK_COUNT];
    mem_pool_t pool;

    pool_init(&pool, buf, BLOCK_SIZE, BLOCK_COUNT);
    printf("pool: %zu blocks, %zu free\n", pool.total_blocks, pool.free_blocks);

    /* Allocate all four blocks. */
    t_slot *a = pool_alloc(&pool);
    t_slot *b = pool_alloc(&pool);
    t_slot *c = pool_alloc(&pool);
    t_slot *d = pool_alloc(&pool);

    if (!a || !b || !c || !d) { fprintf(stderr, "unexpected NULL\n"); return 1; }

    strncpy(a->text, "alpha",   BLOCK_SIZE - 1);
    strncpy(b->text, "beta",    BLOCK_SIZE - 1);
    strncpy(c->text, "gamma",   BLOCK_SIZE - 1);
    strncpy(d->text, "delta",   BLOCK_SIZE - 1);

    printf("allocated: %s  %s  %s  %s\n", a->text, b->text, c->text, d->text);
    printf("free blocks: %zu (expected 0)\n", pool.free_blocks);

    /* Attempt one more — pool is exhausted. */
    t_slot *e = pool_alloc(&pool);
    printf("5th alloc:   %s (expected NULL)\n", e ? e->text : "NULL");

    /* Free two blocks and reallocate. */
    pool_free(&pool, b);
    pool_free(&pool, d);
    printf("free blocks after 2 frees: %zu (expected 2)\n", pool.free_blocks);

    t_slot *x = pool_alloc(&pool);
    t_slot *y = pool_alloc(&pool);
    if (x) strncpy(x->text, "x-reused", BLOCK_SIZE - 1);
    if (y) strncpy(y->text, "y-reused", BLOCK_SIZE - 1);
    printf("reallocated: %s  %s\n", x ? x->text : "NULL", y ? y->text : "NULL");

    /* Return everything. */
    pool_free(&pool, a);
    pool_free(&pool, c);
    pool_free(&pool, x);
    pool_free(&pool, y);
    printf("free blocks at end: %zu (expected %d)\n", pool.free_blocks, BLOCK_COUNT);
    return (0);
}
