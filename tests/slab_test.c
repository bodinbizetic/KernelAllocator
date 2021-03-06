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
    size_t l1Offset = 0;
    tst_OK(get_slab(objSize, &l1Offset, &slab));
    BitMapEntry *bitmap = slab->pBitmap;
    int cnt = 0;

    for (int i = 0; i < slab->numBitMapEntry; i++)
    {
        int n = 1 << sizeof(BitMapEntry) * CHAR_BIT;
        BitMapEntry num = slab->pBitmap[i];
        while (n)
        {
            cnt += num & 1;
            num >>= 1;
            n >>= 1;
        }
    }
    tst_assert(cnt == NUMBER_OF_OBJECTS_IN_SLAB(slab));
}
SLAB_TEST_END

SLAB_TEST_START(full_alloc_slab)
{
    kmem_slab_t *slab;
    size_t l1Offset = 0;
    tst_OK(get_slab(objSize, &l1Offset, &slab));
    tst_assert(slab);
    int i;
    void *last;
    for (i = 0; i < NUMBER_OF_OBJECTS_IN_SLAB(slab); i++)
    {
        tst_OK(slab_allocate(slab, (void **)&last));
    }

    tst_FAIL(slab_allocate(slab, (void **)&last) & SLAB_FULL);
}
SLAB_TEST_END

SLAB_TEST_START(alloc_dealloc)
{
    kmem_slab_t *slab;
    size_t l1Offset = 0;
    tst_OK(get_slab(objSize, &l1Offset, &slab));
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
    size_t l1Offset = 0;
    tst_OK(get_slab(objSize, &l1Offset, &slab));
    const int numBlocks = NUMBER_OF_OBJECTS_IN_SLAB(slab);
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
    kmem_slab_t *slab;
    size_t l1Offset = 0;
    tst_OK(get_slab(1 << 17, &l1Offset, &slab));
    BitMapEntry *bitmap = slab->pBitmap;
    int cnt = 0;

    for (int i = 0; i < slab->numBitMapEntry; i++)
    {
        int n = 1 << sizeof(BitMapEntry) * CHAR_BIT;
        BitMapEntry num = slab->pBitmap[i];
        while (n)
        {
            cnt += num & 1;
            num >>= 1;
            n >>= 1;
        }
    }
    tst_assert(cnt == NUMBER_OF_OBJECTS_IN_SLAB(slab));
}
SLAB_TEST_END

SLAB_TEST_START(kmalloc_test_one)
{
    void *prev = kmalloc(objSize);
    const int entryId = BEST_FIT_BLOCKID(objSize) - 5;
    tst_assert(entryId >= 0 && entryId <= 12);
    tst_assert(prev);

    for (int i = 1; i < _numberOfObjectsInSlab; i++)
    {
        tst_assert(s_bufferHead[entryId].pSlab[FULL] == NULL);
        void *ptr = kmalloc(objSize);
        tst_assert(ptr);
        tst_assert((ptr > prev ? 1 : -1) * ((size_t)ptr - (size_t)prev) % objSize == 0);
        prev = ptr;
    }

    tst_assert(s_bufferHead[entryId].pSlab[FULL] != NULL);
}
SLAB_TEST_END

SLAB_TEST_START(kmalloc_test_lvlup)
{
    const size_t num_pages_to_alloc = 5;
    const int entryId = BEST_FIT_BLOCKID(objSize) - 5;
    if (objSize + sizeof(kmem_slab_t) > BLOCK_SIZE)
        return true;
    for (int j = 0; j < num_pages_to_alloc; j++)
        for (int i = 0; i < _numberOfObjectsInSlab; i++)
        {
            void *ptr = kmalloc(objSize);
            tst_assert(ptr);
        }
    kmem_slab_t *curr = s_bufferHead[entryId].pSlab[FULL];
    int cnt = 0;
    while (curr)
    {
        curr = curr->next;
        cnt++;
    }
    tst_assert(cnt == num_pages_to_alloc);
}
SLAB_TEST_END

SLAB_TEST_START(kmalloc_kfree)
{
    const int entryId = BEST_FIT_BLOCKID(objSize) - 5;
    tst_assert(entryId >= 0 && entryId <= 12);
    void *ptr[BLOCK_SIZE];
    for (int i = 0; i < _numberOfObjectsInSlab; i++)
    {
        tst_assert(s_bufferHead[entryId].pSlab[FULL] == NULL);
        ptr[i] = kmalloc(objSize);
        tst_assert(ptr[i]);
    }

    tst_assert(s_bufferHead[entryId].pSlab[FULL] != NULL);

    for (int i = 0; i < _numberOfObjectsInSlab; i++)
    {
        kfree(ptr[i]);
        tst_assert(s_bufferHead[entryId].pSlab[FULL] == NULL);
    }

    tst_assert(s_bufferHead[entryId].pSlab[HAS_SPACE] == NULL);
    tst_assert(s_bufferHead[entryId].pSlab[EMPTY] != NULL);
}
SLAB_TEST_END

SLAB_TEST_START(l1_cache)
{
    kmem_cache_t *cache = kmem_cache_create("L1_Cache_Test", Obj_Size, NULL, NULL);
    const int ITER = 10;
    const size_t notUsedMemory = (BLOCK_SIZE - sizeof(kmem_slab_t)) % Obj_Size;
    size_t unused = 0;
    const void **ptr = (void **)malloc(ITER * _numberOfObjectsInSlab * sizeof(void *));
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < _numberOfObjectsInSlab; j++)
        {
            ptr[i * _numberOfObjectsInSlab + j] = kmem_cache_alloc(cache);
        }
        unused += CACHE_L1_LINE_SIZE;
        if (unused > notUsedMemory)
        {
            unused = 0;
        }
        tst_assert(unused == cache->l1CacheFiller);
    }

    for (int i = 0; i < ITER; i++)
    {
        for (int j = 0; j < _numberOfObjectsInSlab; j++)
        {
            kmem_cache_free(cache, ptr[i * _numberOfObjectsInSlab + j]);
        }
    }
    tst_assert(unused == cache->l1CacheFiller);

    free(ptr);
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
    SUITE_ADD_OBJSIZE(kmalloc_kfree, Obj_Size);
    SUITE_ADD_OBJSIZE(l1_cache, 300);
}
TEST_SUITE_END