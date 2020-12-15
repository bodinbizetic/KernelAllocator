#include "buddy/buddy.h"
#include "error_codes.h"
#include "helper.h"
#include "slab.h"
#include "slab_impl.h"
#include "tests.h"

extern kmem_buffer_t *s_bufferHead;

SLAB_TEST_START(get_slab_pow_two)
{
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, &slab));
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
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, &slab));
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
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, &slab));
    const int numBlocks = slab->takenSlots;
    for (int i = 0; i < 1000; i++)
    {
        void *ptr;
        tst_OK(slab_allocate(slab, (void **)&ptr));
        tst_OK(slab_free(slab, ptr));
    }
    tst_assert(numBlocks == slab->takenSlots);
}
SLAB_TEST_END

SLAB_TEST_START(alloc_dealloc_mod)
{
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize, &slab));
    const int numBlocks = (slab->slabSize - sizeof(*slab)) / objSize;
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

    tst_assert(0 == slab->takenSlots);

    for (int i = 0; i < numBlocks; i++)
    {
        tst_OK(slab_allocate(slab, (void **)&ptr[i]));
    }
    tst_FAIL(slab_allocate(slab, (void **)&ptr[0]));
    tst_assert(numBlocks == slab->takenSlots);
}
SLAB_TEST_END

SLAB_TEST_START(big_slab_alloc)
{
    const size_t objSize2 = 2 * objSize;
    kmem_slab_t *slab;
    tst_OK(get_slab(objSize2, &slab));
    void **curr = &(slab->free);
    int cnt = 0;
    while (*curr)
    {
        curr = (void *)*curr;
        cnt++;
    }

    tst_assert(cnt == (slab->slabSize - sizeof(kmem_slab_t)) / objSize2);
}
SLAB_TEST_END

SLAB_TEST_START(kmalloc_test_one)
{
    void *prev = kmalloc(objSize);
    tst_assert(prev);
    for (int i = 1; i < (BLOCK_SIZE - sizeof(kmem_slab_t)) / objSize; i++)
    {
        tst_assert(s_bufferHead[0].pSlab[FULL] == NULL);
        void *ptr = kmalloc(objSize);
        tst_assert(ptr);
        tst_assert((size_t)ptr - (size_t)prev == objSize);
        prev = ptr;
    }

    tst_assert(s_bufferHead[0].pSlab[FULL] != NULL);
}
SLAB_TEST_END

SLAB_TEST_START(kmalloc_test_lvlup)
{
    const size_t num_pages_to_alloc = 5;
    for (int j = 0; j < num_pages_to_alloc; j++)
        for (int i = 0; i < (BLOCK_SIZE - sizeof(kmem_slab_t)) / objSize; i++)
        {
            void *ptr = kmalloc(objSize);
            tst_assert(ptr);
        }
    kmem_slab_t *curr = s_bufferHead[0].pSlab[FULL];
    int cnt = 0;
    while (curr)
    {
        curr = curr->next;
        cnt++;
    }
    tst_assert(cnt == num_pages_to_alloc);
}
SLAB_TEST_END

TEST_SUITE_START(slab, 1024)
{
    const size_t Obj_Size = 32;

    SUITE_ADD_OBJSIZE(get_slab_pow_two, Obj_Size);
    SUITE_ADD_OBJSIZE(full_alloc_slab, Obj_Size);
    SUITE_ADD_OBJSIZE(alloc_dealloc, Obj_Size);
    SUITE_ADD_OBJSIZE(alloc_dealloc_mod, Obj_Size);
    SUITE_ADD_OBJSIZE(big_slab_alloc, Obj_Size);
    SUITE_ADD_OBJSIZE(kmalloc_test_one, Obj_Size);
    SUITE_ADD_OBJSIZE(kmalloc_test_lvlup, Obj_Size);
}
TEST_SUITE_END