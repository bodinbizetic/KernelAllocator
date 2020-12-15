#include "buddy/buddy.h"
#include "error_codes.h"
#include "slab_impl.h"

CRESULT get_slab(size_t objectSize, kmem_slab_t **result)
{
    if (objectSize == 0 || !result)
    {
        *result = NULL;
        return PARAM_ERROR;
    }

    size_t sizeOfSlab = BLOCK_SIZE;
    if (objectSize + sizeof(kmem_slab_t) > sizeOfSlab)
    {
        sizeOfSlab = (objectSize + sizeof(kmem_slab_t) - 1) / BLOCK_SIZE + 1;
        sizeOfSlab *= BLOCK_SIZE;
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

    (*result)->takenSlots = 0;
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

CRESULT delete_slab(kmem_slab_t *slab)
{
    if (!slab)
        return PARAM_ERROR;

    if (!slab->next || !slab->prev)
        return SLAB_DELETE_FAIL;

    ASSERT(!buddy_free(slab, slab->slabSize));
}

CRESULT slab_allocate(kmem_slab_t *slab, void **result)
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
    slab->takenSlots++;

    return OK;
}

CRESULT slab_free(kmem_slab_t *slab, void *ptr)
{
    if (!slab || !ptr)
        return PARAM_ERROR;

    const void *slabMemStart = (void *)((size_t)slab + sizeof(kmem_slab_t));

    if (ptr < slabMemStart || ((size_t)ptr - (size_t)slabMemStart) % slab->objectSize != 0)
        return SLAB_DEALLOC_NOT_VALID_ADDRES;

    *(void **)ptr = slab->free;
    slab->free = ptr;
    slab->takenSlots--;

    return OK;
}

CRESULT slab_list_insert(kmem_slab_t **head, kmem_slab_t *slab) // TODO: Insert in sorted order by memory
{
    if (!head || !slab)
    {
        ASSERT(0 && "Slab Param");
        return PARAM_ERROR;
    }

    slab->next = *head;
    slab->prev = NULL;
    if (*head)
    {
        (*head)->prev = slab;
    }
    *head = slab;

    return OK;
}

CRESULT slab_list_delete(kmem_slab_t **head, kmem_slab_t *slab)
{
    if (!head || !slab)
    {
        ASSERT(0 && "Slab Param");
        return PARAM_ERROR;
    }

    if (!slab->prev)
    {
        *head = slab->next;
        if (slab->next)
            slab->next->prev = NULL;
    }
    else
    {
        slab->prev->next = slab->next;
        if (slab->next)
            slab->next->prev = slab->prev;
    }
    slab->next = NULL;
    slab->prev = NULL;

    return OK;
}

CRESULT slab_find_slab_with_obj(kmem_slab_t *head, void *ptr, kmem_slab_t **result)
{
    if (!result || !ptr || !head)
        return PARAM_ERROR;

    kmem_slab_t *curr = head;
    while (curr)
    {
        if (ptr > (void *)curr && ptr < (void *)((size_t)curr + curr->slabSize))
        {
            *result = curr;
            return OK;
        }
    }
    return FAIL;
}