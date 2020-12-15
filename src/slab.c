#include "slab.h"
#include "buddy/buddy.h"
#include "error_codes.h"

#include "slab_impl.h"

#define BUFFER_SIZE_MAX 17
#define BUFFER_SIZE_MIN 5
#define BUFFER_ENTRY_NUM (BUFFER_SIZE_MAX - BUFFER_SIZE_MIN + 1)

int s_errorFlags = OK;
static kmem_cache_t *s_cacheHead;
static kmem_buffer_t *s_bufferHead;

int get_slab(size_t objectSize, kmem_slab_t **result)
{
    if (objectSize == 0 || !result)
    {
        *result = NULL;
        return PARAM_ERROR;
    }

    size_t sizeOfSlab = BLOCK_SIZE;
    if (objectSize + sizeof(kmem_slab_t) > sizeOfSlab)
    {
        sizeOfSlab = (objectSize + sizeof(kmem_slab_t) - 1) / BLOCK_SIZE + BLOCK_SIZE;
    }

    if (objectSize < sizeof(void *))
    {
        objectSize = sizeof(void *);
    }

    int code = buddy_alloc(sizeOfSlab, (void **)result);

    if (code != OK)
    {
        *result = NULL;
        return code;
    }

    (*result)->freeSlots = (sizeOfSlab - sizeof(kmem_slab_t)) / objectSize;
    (*result)->free = NULL;
    (*result)->next = NULL;
    (*result)->prev = NULL;
    (*result)->objectSize = objectSize;
    (*result)->slabSize = sizeOfSlab;

    const void *start = (void *)((size_t)*result + sizeof(kmem_slab_t));
    void **prev = NULL;
    for (size_t i = (size_t)start; i + objectSize < (size_t)*result + sizeOfSlab; i += objectSize)
    {
        if (!prev)
            (*result)->free = (void *)i;
        else
            *prev = (void *)i;
        prev = (void **)i;
    }
    if (prev)
        *prev = NULL;

    return OK;
}

int slab_allocate(kmem_slab_t *slab, void **result)
{
    if (!slab)
    {
        *result = NULL;
        return PARAM_ERROR;
    }
    if (!slab->free)
    {
        *result = NULL;
        return SLAB_FULL;
    }

    *result = slab->free;
    slab->free = *(void **)*result;
    slab->freeSlots--;

    return OK;
}

int slab_free(kmem_slab_t *slab, void *ptr)
{
    if (!slab || !ptr)
        return PARAM_ERROR;

    const void *slabMemStart = (size_t)slab + sizeof(kmem_slab_t);

    if (ptr < slabMemStart || ((size_t)ptr - (size_t)slabMemStart) % slab->objectSize != 0)
        return SLAB_DEALLOC_NOT_VALID_ADDRES;

    *(void **)ptr = slab->free;
    slab->free = ptr;
    slab->freeSlots++;

    return OK;
}

static void kmem_buffer_init()
{
    if (!s_bufferHead)
        return;

    for (int i = 0; i < BUFFER_ENTRY_NUM; i++)
    {
        s_bufferHead[i].size = 1 << i + BUFFER_SIZE_MIN;
        s_bufferHead[i].pSlabEmpty = NULL;
        s_bufferHead[i].pSlabFull = NULL;
        s_bufferHead[i].pSlabHasSpace = NULL;
    }
    SLAB_LOG("Initialized buffer memory %ld\n", s_bufferHead);
}

void kmem_init(void *space, int block_num)
{
    ASSERT(BUFFER_SIZE_MAX >= BUFFER_SIZE_MIN);
    s_errorFlags = NULL;
    s_cacheHead = NULL;

    int code = buddy_init(space, (size_t)block_num * BLOCK_SIZE);
    code |= buddy_alloc(sizeof(kmem_buffer_t) * BUFFER_ENTRY_NUM, (void **)&s_bufferHead);

    if (code == OK)
    {
        kmem_buffer_init();
    }
}

void *kmalloc(size_t size)
{
    const int entryId = BEST_FIT_BLOCKID(size) - 5;

    if (s_bufferHead[entryId].pSlabHasSpace)
    {
    }

    // kmem_slab_t *slab = get_slab(size);
    /*if (!slab)
        return NULL;
*/
    return NULL;
}