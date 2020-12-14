#include "buddy/buddy.h"
#include "error_codes.h"
#include "helper.h"
#include "slab.h"
#include "slab_impl.h"
#include "tests.h"

extern int s_errorFlags;

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
    void *first;
    tst_OK(slab_allocate(slab, (void **)&first));
    void *last;
    for (i = 1; i < (BLOCK_SIZE - sizeof(kmem_slab_t)) / objSize; i++)
    {
        tst_OK(slab_allocate(slab, (void **)&last)) tst_OK(s_errorFlags);
    }
    int x;
    tst_FAIL(slab_allocate(slab, (void **)&last) & SLAB_FULL);
}
SLAB_TEST_END

TEST_SUITE_START(slab, 1024)
{
    SUITE_ADD(get_slab_pow_two);
    SUITE_ADD(full_alloc_slab);
}
TEST_SUITE_END