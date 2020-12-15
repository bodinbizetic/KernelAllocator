#include "slab.h"
#include "buddy/buddy.h"
#include "error_codes.h"

#include "slab_impl.h"

#define BUFFER_SIZE_MAX 17
#define BUFFER_SIZE_MIN 5
#define BUFFER_ENTRY_NUM (BUFFER_SIZE_MAX - BUFFER_SIZE_MIN + 1)

static kmem_cache_t *s_cacheHead;
kmem_buffer_t *s_bufferHead;

static void kmem_buffer_init()
{
    if (!s_bufferHead)
        return;

    for (int i = 0; i < BUFFER_ENTRY_NUM; i++)
    {
        s_bufferHead[i].size = 1 << i + BUFFER_SIZE_MIN;
        s_bufferHead[i].pSlab[FULL] = NULL;
        s_bufferHead[i].pSlab[HAS_SPACE] = NULL;
        s_bufferHead[i].pSlab[EMPTY] = NULL;
    }
    SLAB_LOG("Initialized buffer memory %ld\n", s_bufferHead);
}

void kmem_init(void *space, int block_num)
{
    ASSERT(BUFFER_SIZE_MAX >= BUFFER_SIZE_MIN);
    s_cacheHead = NULL;

    int code = buddy_init(space, (size_t)block_num * BLOCK_SIZE);
    code |= buddy_alloc(sizeof(kmem_buffer_t) * BUFFER_ENTRY_NUM, (void **)&s_bufferHead);

    if (code == OK)
    {
        kmem_buffer_init();
    }
}

static void *slab_allocate_has_space(kmem_slab_t *pSlab[])
{
    if (!pSlab || !pSlab[HAS_SPACE])
        return NULL;

    void *result;
    kmem_slab_t *slab = pSlab[HAS_SPACE];
    CRESULT code = slab_allocate(slab, &result);
    if (code != OK)
    {
        ASSERT(0 && "allocate failed");
        return NULL;
    }
    if (!pSlab[HAS_SPACE]->free)
    {
        ASSERT(slab->takenSlots == ((slab->slabSize - sizeof(*slab)) / slab->objectSize));
        slab_list_delete(&pSlab[HAS_SPACE], slab);
        slab_list_insert(&pSlab[FULL], slab);
    }

    return result;
}

static void *slab_allocate_empty(kmem_slab_t *pSlab[])
{
    if (!pSlab || !pSlab[EMPTY])
        return NULL;

    if (pSlab[EMPTY])
    {
        kmem_slab_t *slab = pSlab[EMPTY];
        slab_list_delete(&pSlab[EMPTY], slab);
        slab_list_insert(&pSlab[HAS_SPACE], slab);
    }

    return slab_allocate_has_space(pSlab);
}

static void *slab_allocate_object(kmem_slab_t *pSlab[], size_t objSize)
{
    if (pSlab[HAS_SPACE])
    {
        return slab_allocate_has_space(pSlab);
    }

    if (pSlab[EMPTY])
    {
        return slab_allocate_empty(pSlab);
    }
    else
    {
        kmem_slab_t *slab;
        CRESULT code = get_slab(objSize, &slab);
        if (code != OK)
            return NULL;

        slab_list_insert(&pSlab[HAS_SPACE], slab);
        return slab_allocate_has_space(pSlab);
    }

    return NULL;
}

void *kmalloc(size_t size)
{
    const int entryId = BEST_FIT_BLOCKID(size) - 5;
    return slab_allocate_object(s_bufferHead[entryId].pSlab, 1 << BEST_FIT_BLOCKID(size));
}
