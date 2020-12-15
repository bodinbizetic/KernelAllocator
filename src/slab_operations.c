#include "error_codes.h"
#include "slab_impl.h"

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
