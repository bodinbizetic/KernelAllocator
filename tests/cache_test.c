#include "buddy/buddy.h"
#include "helper.h"
#include "slab_impl.h"
#include "tests.h"

extern kmem_cache_t *s_cacheHead;

SLAB_TEST_START(cache_create)
{
    kmem_cache_t *ptr[BLOCK_SIZE];
    const int ITER = 236;
    for (int i = 0; i < ITER; i++)
    {
        char c[30];
        ptr[i] = kmem_cache_create(_itoa(i, c, 10), 100, NULL, NULL);
        tst_assert(ptr[i]);
        tst_OK(s_cacheHead->errorFlags);
    }
    int cnt = 0;
    for (enum Slab_Type status = EMPTY; status <= FULL; status++)
        for (kmem_slab_t *start = s_cacheHead->pSlab[status]; start; start = start->next)
        {
            cnt += NUMBER_OF_OBJECTS_IN_SLAB(start);
        }
    tst_assert(cnt >= ITER);
}
SLAB_TEST_END

SLAB_TEST_START(cache_create_delete)
{
    kmem_cache_t *ptr[BLOCK_SIZE];
    const int ITER = 236;
    for (int i = 0; i < ITER; i++)
    {
        char c[30];
        ptr[i] = kmem_cache_create(_itoa(i, c, 10), 100, NULL, NULL);
        tst_assert(ptr[i]);
        tst_OK(s_cacheHead->errorFlags);
    }
    for (int i = 0; i < ITER; i++)
    {
        kmem_cache_destroy(ptr[i]);
        tst_OK(s_cacheHead->errorFlags);
    }

    int cnt = 0;
    for (enum Slab_Type status = HAS_SPACE; status <= FULL; status++)
        for (kmem_slab_t *start = s_cacheHead->pSlab[status]; start; start = start->next)
        {
            cnt += NUMBER_OF_OBJECTS_IN_SLAB(start);
        }
    tst_assert(cnt == 0);
    tst_assert(s_cacheHead->pSlab[EMPTY]);
}
SLAB_TEST_END

static int Destructor_Count = 0;
void destructor(void *ptr)
{
    Destructor_Count++;
}

SLAB_TEST_START(cache_create_alloc_delete_destructor)
{
    kmem_cache_t *cache;
    const int ITER = 236;
    cache = kmem_cache_create("Name", objSize, NULL, destructor);
    tst_assert(cache);
    tst_OK(s_cacheHead->errorFlags);
    void *objects[BLOCK_SIZE];
    for (int i = 0; i < ITER; i++)
    {
        objects[i] = kmem_cache_alloc(cache);
        tst_assert(objects[i]);
        tst_OK(cache->errorFlags);
    }

    Destructor_Count = 0;
    for (int i = 0; i < ITER; i++)
    {
        kmem_cache_free(cache, objects[i]);
        tst_OK(cache->errorFlags);
    }
    kmem_cache_destroy(cache);
    const int checkSum =
        ITER - ITER % _numberOfObjectsInSlab + _numberOfObjectsInSlab * (ITER % _numberOfObjectsInSlab ? 1 : 0);
    tst_assert(Destructor_Count == checkSum);
}
SLAB_TEST_END

TEST_SUITE_START(cache, 1024 * 16)
{
    const size_t Obj_Size = 32;
    SUITE_ADD_OBJSIZE(cache_create, Obj_Size);
    SUITE_ADD_OBJSIZE(cache_create_delete, Obj_Size);
    SUITE_ADD_OBJSIZE(cache_create_alloc_delete_destructor, Obj_Size);
}
TEST_SUITE_END
