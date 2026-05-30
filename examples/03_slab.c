/**
 * examples/03_slab.c
 *
 * Demonstrates the slab allocator:
 *   - Manages multiple fixed-size pools internally.
 *   - slab_alloc() picks the smallest pool that fits the request.
 *   - slab_free() returns the block to the correct pool automatically.
 *
 * Build:  make examples
 * Run:    ./examples/bin/03_slab
 */

#include <stdio.h>
#include <string.h>
#include "libmem.h"

/* Three pools: 16-byte, 64-byte, 256-byte blocks, 4 blocks each. */
#define SMALL_SIZE   16
#define MEDIUM_SIZE  64
#define LARGE_SIZE  256
#define COUNT         4

int main(void)
{
    static unsigned char small_buf [SMALL_SIZE  * COUNT];
    static unsigned char medium_buf[MEDIUM_SIZE * COUNT];
    static unsigned char large_buf [LARGE_SIZE  * COUNT];

    mem_slab_config_t configs[] = {
        { small_buf,  SMALL_SIZE,  COUNT },
        { medium_buf, MEDIUM_SIZE, COUNT },
        { large_buf,  LARGE_SIZE,  COUNT },
    };

    mem_slab_t slab;
    slab_init(&slab, configs, 3);
    printf("slab ready with %zu pools\n", slab.count);

    /* Allocate objects of different sizes — slab picks the right pool. */
    char *tiny  = slab_alloc(&slab, 8);          /* → small pool  (16 B) */
    char *mid   = slab_alloc(&slab, 48);         /* → medium pool (64 B) */
    char *big   = slab_alloc(&slab, 200);        /* → large pool (256 B) */

    if (!tiny || !mid || !big) { fprintf(stderr, "slab exhausted\n"); return 1; }

    strncpy(tiny, "tiny",   SMALL_SIZE  - 1);
    strncpy(mid,  "medium", MEDIUM_SIZE - 1);
    strncpy(big,  "large",  LARGE_SIZE  - 1);

    printf("tiny='%s'  mid='%s'  big='%s'\n", tiny, mid, big);

    /* Request that exceeds the largest pool — returns NULL. */
    void *too_big = slab_alloc(&slab, 512);
    printf("over-size alloc: %s (expected NULL)\n", too_big ? "non-NULL" : "NULL");

    /* Return blocks; slab matches by block_size, not by pointer value. */
    slab_free(&slab, tiny);
    slab_free(&slab, mid);
    slab_free(&slab, big);

    /* Verify that the freed blocks are reusable. */
    char *reuse = slab_alloc(&slab, 8);
    if (reuse) strncpy(reuse, "reused", SMALL_SIZE - 1);
    printf("reuse after free: '%s'\n", reuse ? reuse : "NULL");
    if (reuse) slab_free(&slab, reuse);

    return (0);
}
