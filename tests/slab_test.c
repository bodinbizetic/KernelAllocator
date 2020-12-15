#include "buddy/buddy.h"
#include "error_codes.h"
#include "helper.h"
#include "slab.h"
#include "slab_impl.h"
#include "tests.h"

SLAB_TEST_START(get_slab_pow_two)
{
    const size_t objSize = 32;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, (void **)&slab));
    void **curr = &(slab->free);
    int cnt = 0;
    while (*curr)
    {
        curr = (void *)*curr;
        cnt++;
    }

    tst_assert(cnt == (BLOCK_SIZE - sizeof(kmem_slab_t)) / objSize);
}
SLAB_TEST_END

SLAB_TEST_START(full_alloc_slab)
{
    const size_t objSize = 32;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, (void **)&slab));
    tst_assert(slab);
    int i;
    void *last;
    for (i = 0; i < (BLOCK_SIZE - sizeof(kmem_slab_t)) / objSize; i++)
    {
        tst_OK(slab_allocate(slab, (void **)&last));
    }
    int x;
    tst_FAIL(slab_allocate(slab, (void **)&last) & SLAB_FULL);
}
SLAB_TEST_END

SLAB_TEST_START(alloc_dealloc)
{
    const size_t objSize = 32;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, (void **)&slab));
    const int numBlocks = slab->freeSlots;
    for (int i = 0; i < 1000; i++)
    {
        void *ptr;
        tst_OK(slab_allocate(slab, (void **)&ptr));
        tst_OK(slab_free(slab, ptr));
    }
    tst_assert(numBlocks == slab->freeSlots);
}
SLAB_TEST_END

SLAB_TEST_START(alloc_dealloc_mod)
{
    const size_t objSize = 32;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, (void **)&slab));
    const int numBlocks = slab->freeSlots;
    void *ptr[BLOCK_SIZE];
    for (int i = 0; i < numBlocks; i++)
    {
        tst_OK(slab_allocate(slab, (void **)&ptr[i]));
    }
    const int MOD = 3;
    for (int i = 0; i < MOD; i++)
    {
        for (int j = 0; j < numBlocks; j++)
        {
            if (j % MOD == i)
            {
                tst_OK(slab_free(slab, ptr[j]));
            }
        }
    }

    tst_assert(numBlocks == slab->freeSlots);

    for (int i = 0; i < numBlocks; i++)
    {
        tst_OK(slab_allocate(slab, (void **)&ptr[i]));
    }
    tst_FAIL(slab_allocate(slab, (void **)&ptr[0]));
    tst_assert(0 == slab->freeSlots);
}
SLAB_TEST_END

SLAB_TEST_START(big_slab_alloc)
{
    const size_t objSize = 2 * BLOCK_SIZE;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, (void **)&slab));
    void **curr = &(slab->free);
    int cnt = 0;
    while (*curr)
    {
        curr = (void *)*curr;
        cnt++;
    }

    tst_assert(cnt == (slab->slabSize - sizeof(kmem_slab_t)) / objSize);
}
SLAB_TEST_END

TEST_SUITE_START(slab, 1024)
{
    SUITE_ADD(get_slab_pow_two);
    SUITE_ADD(full_alloc_slab);
    SUITE_ADD(alloc_dealloc);
    SUITE_ADD(alloc_dealloc_mod);
    SUITE_ADD(big_slab_alloc);
}
TEST_SUITE_END