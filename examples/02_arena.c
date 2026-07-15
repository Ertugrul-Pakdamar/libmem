/**
 * examples/02_arena.c
 *
 * Demonstrates the arena (bump) allocator:
 *   - Sequential bump-allocations in O(1), automatically aligned.
 *   - No individual frees — arena_reset() reclaims everything at once.
 *   - arena_mark() / arena_rewind() for lightweight partial reclamation.
 *   - arena_used() / arena_remaining() for capacity introspection.
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
    printf("arena capacity : %d bytes\n", ARENA_SIZE);

    /* ------------------------------------------------------------------ */
    /* 1. Basic bump-allocation of mixed types.                            */
    /* ------------------------------------------------------------------ */
    int    *counter = arena_alloc(&arena, sizeof(int));
    double *ratio   = arena_alloc(&arena, sizeof(double));
    char   *label   = arena_alloc(&arena, 32);

    if (!counter || !ratio || !label)
    {
        fprintf(stderr, "arena exhausted during initial allocs\n");
        return (1);
    }

    *counter = 42;
    *ratio   = 3.14159;
    strncpy(label, "arena-example", 31);

    printf("counter=%-4d  ratio=%.5f  label=%s\n", *counter, *ratio, label);
    printf("used after 3 allocs : %zu bytes\n", arena_used(&arena));
    printf("remaining           : %zu bytes\n", arena_remaining(&arena));

    /* ------------------------------------------------------------------ */
    /* 2. Mark / rewind — temporary allocations that are cheaply reclaimed */
    /* ------------------------------------------------------------------ */
    mem_arena_mark_t mark = arena_mark(&arena);
    printf("\n-- mark saved at offset %zu --\n", mark);

    /* Temporary allocations (e.g. per-frame scratch data). */
    char *scratch = arena_alloc(&arena, 64);
    if (scratch)
        strncpy(scratch, "temporary scratch buffer", 63);
    printf("used with scratch   : %zu bytes\n", arena_used(&arena));

    /* Discard all allocations made after 'mark' in O(1). */
    arena_rewind(&arena, mark);
    printf("used after rewind   : %zu bytes  (back to %zu)\n",
        arena_used(&arena), mark);
    printf("remaining after rwd : %zu bytes\n", arena_remaining(&arena));

    /* ------------------------------------------------------------------ */
    /* 3. Full reset — reclaims the entire arena.                          */
    /* ------------------------------------------------------------------ */
    arena_reset(&arena);
    printf("\nused after reset    : %zu bytes\n", arena_used(&arena));

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
