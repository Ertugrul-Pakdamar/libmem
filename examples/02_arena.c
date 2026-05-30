/**
 * examples/02_arena.c
 *
 * Demonstrates the arena (bump) allocator:
 *   - Sequential bump-allocations in O(1).
 *   - No individual frees — arena_reset() reclaims everything at once.
 *   - Suitable for per-frame or per-request scratch memory.
 *
 * Build:  make examples
 * Run:    ./examples/bin/02_arena
 */

#include <stdio.h>
#include <string.h>
#include "libmem.h"

#define ARENA_SIZE 256

int main(void)
{
    static unsigned char buf[ARENA_SIZE];
    mem_arena_t arena;

    arena_init(&arena, buf, ARENA_SIZE);
    printf("arena capacity: %zu bytes\n", arena.size);

    /* Bump-allocate several objects of different sizes. */
    int    *counter = arena_alloc(&arena, sizeof(int));
    double *ratio   = arena_alloc(&arena, sizeof(double));
    char   *label   = arena_alloc(&arena, 32);

    if (!counter || !ratio || !label) { fprintf(stderr, "arena exhausted\n"); return 1; }

    *counter = 42;
    *ratio   = 3.14159;
    strncpy(label, "arena-example", 31);

    printf("counter=%d  ratio=%.5f  label=%s\n", *counter, *ratio, label);
    printf("offset after 3 allocs: %zu bytes\n", arena.offset);

    /* Reset reclaims the entire arena in O(1). */
    arena_reset(&arena);
    printf("offset after reset:    %zu bytes\n", arena.offset);

    /* Re-use the arena for a second round. */
    int *values = arena_alloc(&arena, sizeof(int) * 8);
    if (values)
    {
        for (int i = 0; i < 8; i++) values[i] = (i + 1) * 10;
        printf("second round values:");
        for (int i = 0; i < 8; i++) printf(" %d", values[i]);
        printf("\n");
    }

    /* No destructor needed — arena is backed by the stack buffer. */
    return (0);
}
