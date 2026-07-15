/**
 * tests/test_libmem.c
 *
 * libmem regression test suite.
 * Covers all corner cases identified in code review.
 *
 * Build and run:
 *   make test          (release)
 *   make test-debug    (debug — also covers double-free detection paths)
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "libmem.h"

/* ---- Minimal test framework --------------------------------------------- */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  \033[32mPASS\033[0m  %s\n", msg); \
        g_pass++; \
    } else { \
        printf("  \033[31mFAIL\033[0m  %s  (line %d)\n", msg, __LINE__); \
        g_fail++; \
    } \
} while (0)

/* ---- Arena tests --------------------------------------------------------- */

static void test_arena_basic(void)
{
    puts("\n[arena] Basic allocation and introspection");
    static unsigned char buf[256];
    mem_arena_t a;
    arena_init(&a, buf, 256);

    void *p1 = arena_alloc(&a, 16);
    void *p2 = arena_alloc(&a, 32);
    ASSERT(p1 != NULL,              "alloc 16 bytes succeeds");
    ASSERT(p2 != NULL,              "alloc 32 bytes succeeds");
    ASSERT(p2 > p1,                 "sequential ptrs advance");
    ASSERT(arena_used(&a) > 0,      "used > 0 after allocs");
    ASSERT(arena_remaining(&a) < 256, "remaining < capacity after allocs");
    ASSERT(arena_used(&a) + arena_remaining(&a) == 256, "used + remaining == capacity");
}

static void test_arena_misaligned_buffer(void)
{
    puts("\n[arena] Misaligned buffer — returned pointers must still be aligned");
    static unsigned char raw[256 + 16];
    unsigned char *misaligned = raw + 1; /* force misaligned start */
    mem_arena_t a;
    arena_init(&a, misaligned, 255);

    int    *pi = arena_alloc(&a, sizeof(int));
    double *pd = arena_alloc(&a, sizeof(double));
    ASSERT(pi != NULL, "alloc int from misaligned buf");
    ASSERT(pd != NULL, "alloc double from misaligned buf");
    ASSERT(((uintptr_t)pi % _Alignof(int))    == 0, "int* properly aligned");
    ASSERT(((uintptr_t)pd % _Alignof(double)) == 0, "double* properly aligned");
}

static void test_arena_invalid_alignment(void)
{
    puts("\n[arena] Non-power-of-two and zero alignment must return NULL");
    static unsigned char buf[256];
    mem_arena_t a;
    arena_init(&a, buf, 256);
    size_t used_before = arena_used(&a);

    ASSERT(arena_alloc_aligned(&a, 8, 0) == NULL, "align=0  → NULL");
    ASSERT(arena_alloc_aligned(&a, 8, 3) == NULL, "align=3  → NULL");
    ASSERT(arena_alloc_aligned(&a, 8, 6) == NULL, "align=6  → NULL");
    ASSERT(arena_alloc_aligned(&a, 8, 5) == NULL, "align=5  → NULL");
    ASSERT(arena_used(&a) == used_before,          "cursor unchanged after bad allocs");
    ASSERT(arena_alloc_aligned(&a, 8, 1) != NULL,  "align=1  → non-NULL");
    ASSERT(arena_alloc_aligned(&a, 8, 4) != NULL,  "align=4  → non-NULL");
}

static void test_arena_zero_size(void)
{
    puts("\n[arena] Zero-size allocation must return NULL without moving cursor");
    static unsigned char buf[64];
    mem_arena_t a;
    arena_init(&a, buf, 64);
    ASSERT(arena_alloc(&a, 0) == NULL, "arena_alloc(0) → NULL");
    ASSERT(arena_used(&a) == 0,        "cursor unchanged");
}

static void test_arena_exhaustion(void)
{
    puts("\n[arena] Exhaustion and exact-boundary allocation");
    static unsigned char buf[64];
    mem_arena_t a;
    arena_init(&a, buf, 64);

    void *p = arena_alloc_aligned(&a, 64, 1);
    ASSERT(p  != NULL, "exact 64-byte alloc succeeds");
    ASSERT(arena_alloc_aligned(&a, 1, 1) == NULL, "next alloc → NULL (exhausted)");

    arena_reset(&a);
    ASSERT(arena_used(&a) == 0,      "used == 0 after reset");
    ASSERT(arena_remaining(&a) == 64, "remaining == 64 after reset");
}

static void test_arena_mark_rewind(void)
{
    puts("\n[arena] Mark / rewind");
    static unsigned char buf[256];
    mem_arena_t a;
    arena_init(&a, buf, 256);

    arena_alloc(&a, 32);
    mem_arena_mark_t mark      = arena_mark(&a);
    size_t           used_mark = arena_used(&a);

    arena_alloc(&a, 64);
    ASSERT(arena_used(&a) > used_mark, "used grew after mark");

    arena_rewind(&a, mark);
    ASSERT(arena_used(&a) == used_mark, "rewind restores cursor exactly");

    /* Forward rewind must be silently ignored */
    arena_rewind(&a, used_mark + 100);
    ASSERT(arena_used(&a) == used_mark, "forward rewind is ignored");

    arena_reset(&a);
    ASSERT(arena_used(&a) == 0, "reset after rewind works");
}

/* ---- Pool tests ---------------------------------------------------------- */

static void test_pool_basic(void)
{
    puts("\n[pool] Basic alloc / free / exhaustion");
    static unsigned char buf[32 * 4];
    mem_pool_t pool;
    pool_init(&pool, buf, 32, 4);

    ASSERT(pool.total_blocks == 4, "total_blocks == 4");
    ASSERT(pool.free_blocks  == 4, "free_blocks  == 4 at init");

    void *a = pool_alloc(&pool);
    void *b = pool_alloc(&pool);
    ASSERT(a != NULL && b != NULL, "first two allocs succeed");
    ASSERT(pool_used(&pool)      == 2, "pool_used == 2");
    ASSERT(pool_remaining(&pool) == 2, "pool_remaining == 2");

    pool_free(&pool, a);
    ASSERT(pool.free_blocks == 3, "free_blocks == 3 after one free");
    pool_free(&pool, b);
    ASSERT(pool.free_blocks == 4, "all blocks returned");

    /* Exhaust */
    pool_alloc(&pool); pool_alloc(&pool);
    pool_alloc(&pool); pool_alloc(&pool);
    ASSERT(pool_alloc(&pool) == NULL, "5th alloc → NULL (exhausted)");
}

static void test_pool_invalid_init(void)
{
    puts("\n[pool] Invalid pool_init must set free_blocks and total_blocks to 0");
    static unsigned char buf[128];
    mem_pool_t p;

    pool_init(&p, buf, 16, 0);   /* block_count == 0 */
    ASSERT(p.free_blocks  == 0, "block_count=0 → free_blocks=0");
    ASSERT(p.total_blocks == 0, "block_count=0 → total_blocks=0");
    ASSERT(pool_alloc(&p) == NULL, "alloc from count=0 pool → NULL");

    pool_init(&p, buf, 3, 4);    /* block_size not multiple of sizeof(void*) */
    ASSERT(p.free_blocks == 0, "bad block_size(3) → free_blocks=0");

    pool_init(&p, buf, 4, 4);    /* block_size < sizeof(void*) on 64-bit */
    /* May pass on 32-bit (sizeof(void*)==4); only check for invalid on 64-bit */
    if (sizeof(void *) > 4)
        ASSERT(p.free_blocks == 0, "block_size < sizeof(void*) → free_blocks=0");
}

static void test_pool_interior_ptr(void)
{
    puts("\n[pool] Interior-pointer free must be silently rejected");
    static unsigned char buf[32 * 4];
    mem_pool_t pool;
    pool_init(&pool, buf, 32, 4);

    void  *blk    = pool_alloc(&pool);
    size_t before = pool.free_blocks;
    ASSERT(blk != NULL, "alloc succeeds");

    pool_free(&pool, (unsigned char *)blk + 1);
    ASSERT(pool.free_blocks == before, "interior ptr +1: free_blocks unchanged");

    pool_free(&pool, (unsigned char *)blk + 8);
    ASSERT(pool.free_blocks == before, "interior ptr +8: free_blocks unchanged");

    pool_free(&pool, blk);  /* valid free */
    ASSERT(pool.free_blocks == before + 1, "valid free accepted");
}

static void test_pool_null_and_oor(void)
{
    puts("\n[pool] NULL and out-of-range pointers are silently ignored");
    static unsigned char buf[32 * 4];
    static unsigned char other[32];
    mem_pool_t pool;
    pool_init(&pool, buf, 32, 4);
    pool_alloc(&pool);
    size_t before = pool.free_blocks;

    pool_free(&pool, NULL);
    ASSERT(pool.free_blocks == before, "NULL ptr: free_blocks unchanged");

    pool_free(&pool, other);
    ASSERT(pool.free_blocks == before, "out-of-range ptr: free_blocks unchanged");
}

#ifdef LIBMEM_DEBUG
static void test_pool_double_free(void)
{
    puts("\n[pool] Double-free detected in debug mode");
    static unsigned char buf[32 * 4];
    mem_pool_t pool;
    pool_init(&pool, buf, 32, 4);

    void  *blk    = pool_alloc(&pool);
    pool_free(&pool, blk);
    size_t after1 = pool.free_blocks;
    pool_free(&pool, blk);  /* double-free — should print to stderr and be rejected */
    ASSERT(pool.free_blocks == after1, "double-free: free_blocks unchanged");
}
#endif

/* ---- Slab tests ---------------------------------------------------------- */

static void test_slab_basic(void)
{
    puts("\n[slab] Basic allocation and pool selection");
    static unsigned char s[16 * 4], m[64 * 4], l[256 * 4];
    mem_slab_config_t cfgs[] = {{ s, 16, 4 }, { m, 64, 4 }, { l, 256, 4 }};
    mem_slab_t slab;
    slab_init(&slab, cfgs, 3);

    void *p8  = slab_alloc(&slab, 8);
    void *p48 = slab_alloc(&slab, 48);
    void *p96 = slab_alloc(&slab, 96);
    ASSERT(p8  != NULL, "slab_alloc(8)   → non-NULL (16-byte pool)");
    ASSERT(p48 != NULL, "slab_alloc(48)  → non-NULL (64-byte pool)");
    ASSERT(p96 != NULL, "slab_alloc(96)  → non-NULL (256-byte pool)");
    ASSERT(slab_alloc(&slab, 512) == NULL, "slab_alloc(512) → NULL (oversized)");

    slab_free(&slab, p8);
    slab_free(&slab, p48);
    slab_free(&slab, p96);
}

static void test_slab_invalid_pool_fallback(void)
{
    puts("\n[slab] Invalid small pool must not block valid larger pool");
    static unsigned char valid_s[16 * 4];
    static unsigned char bad_buf[15 * 4]; /* block_size=15: not mult of sizeof(void*) */
    static unsigned char valid_l[32 * 4];

    mem_slab_config_t cfgs[] = {
        { valid_s, 16, 4 },
        { bad_buf, 15, 4 }, /* pool_init will mark this invalid */
        { valid_l, 32, 4 },
    };
    mem_slab_t slab;
    slab_init(&slab, cfgs, 3);

    /* 20 > 16, so 16-pool is too small; 15-pool is invalid; must reach 32-pool */
    void *p = slab_alloc(&slab, 20);
    ASSERT(p != NULL, "slab_alloc(20) falls through to 32-byte pool");
    if (p) slab_free(&slab, p);
}

static void test_slab_interior_ptr(void)
{
    puts("\n[slab] Interior-pointer slab_free is rejected (via pool_free)");
    static unsigned char buf[32 * 4];
    mem_slab_config_t cfgs[] = {{ buf, 32, 4 }};
    mem_slab_t slab;
    slab_init(&slab, cfgs, 1);

    void  *blk    = slab_alloc(&slab, 16);
    size_t before = slab.pools[0].free_blocks;
    ASSERT(blk != NULL, "slab_alloc succeeds");

    slab_free(&slab, (unsigned char *)blk + 1);
    ASSERT(slab.pools[0].free_blocks == before, "interior slab_free rejected");

    slab_free(&slab, blk);
    ASSERT(slab.pools[0].free_blocks == before + 1, "valid slab_free accepted");
}

/* ---- main --------------------------------------------------------------- */

int main(void)
{
#ifdef LIBMEM_DEBUG
    printf("\033[33m\nlibmem test suite — debug build (LIBMEM_DEBUG)\033[0m\n");
#else
    printf("\033[36m\nlibmem test suite — release build\033[0m\n");
#endif

    test_arena_basic();
    test_arena_misaligned_buffer();
    test_arena_invalid_alignment();
    test_arena_zero_size();
    test_arena_exhaustion();
    test_arena_mark_rewind();

    test_pool_basic();
    test_pool_invalid_init();
    test_pool_interior_ptr();
    test_pool_null_and_oor();
#ifdef LIBMEM_DEBUG
    test_pool_double_free();
#endif

    test_slab_basic();
    test_slab_invalid_pool_fallback();
    test_slab_interior_ptr();

    int total = g_pass + g_fail;
    printf("\n%d/%d tests passed", g_pass, total);
    if (g_fail)
        printf("  \033[31m(%d FAILED)\033[0m", g_fail);
    printf("\n\n");
    return (g_fail > 0 ? 1 : 0);
}
