#include "buddy/buddy.h"
#include "error_codes.h"
#include "slab_impl.h"

static inline void setBitMap(kmem_slab_t *slab, int id, uint8_t value)
{
    const int addr = id >> BITMAP_NUM_BITS_ENTRY_POW_2;
    const int off = 1 << (id & ((1 << BITMAP_NUM_BITS_ENTRY_POW_2) - 1));
    slab->pBitmap[addr] = (value ? slab->pBitmap[addr] | off : slab->pBitmap[addr] & ~off);
}

static inline uint8_t getBitMap(kmem_slab_t *slab, int id)
{
    const int addr = id >> BITMAP_NUM_BITS_ENTRY_POW_2;
    const int off = 1 << (id & ((1 << BITMAP_NUM_BITS_ENTRY_POW_2) - 1));
    return slab->pBitmap[addr] & off;
}

CRESULT get_slab_init_bitmap(kmem_slab_t *slab)
{
    if (!slab)
        return PARAM_ERROR;

    slab->numBitMapEntry =
        (slab->slabSize - sizeof(kmem_slab_t)) / (slab->objectSize * CHAR_BIT * sizeof(BitMapEntry)) + 1;
    slab->pBitmap = slab->memStart;
    slab->memStart = (void *)((size_t)slab->memStart + slab->numBitMapEntry * sizeof(BitMapEntry));

    for (int i = 0; i < slab->numBitMapEntry; i++)
    {
        slab->pBitmap[i] = 0;
    }

    return OK;
}

CRESULT get_slab(size_t objectSize, kmem_slab_t **result, function constructor)
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
        sizeOfSlab = (1 << BEST_FIT_BLOCKID(sizeOfSlab)) * BLOCK_SIZE; // TODO: Check
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
    (*result)->next = NULL;
    (*result)->prev = NULL;
    (*result)->objectSize = objectSize;
    (*result)->slabSize = sizeOfSlab;
    (*result)->pBitmap = NULL;
    (*result)->memStart = (void *)((size_t)(*result) + sizeof(kmem_slab_t));

    get_slab_init_bitmap(*result);

    const void *start = (*result)->memStart;
    for (size_t i = (size_t)start; i + objectSize < (size_t)*result + sizeOfSlab; i += objectSize)
    {
        if (constructor != NULL)
            constructor((void *)i);
        setBitMap((*result), (i - (size_t)start) / (*result)->objectSize, 1);
    }

    return OK;
}

CRESULT delete_slab(kmem_slab_t *slab, function destructor)
{
    if (!slab)
        return PARAM_ERROR;

    if (!slab->next || !slab->prev)
        return SLAB_DELETE_FAIL;

    if (destructor)
    {
        for (size_t i = (size_t)slab->memStart; i + slab->objectSize < (size_t)slab + slab->slabSize;
             i += slab->objectSize)
        {
            destructor((void *)i);
        }
    }

    buddy_free(slab, slab->slabSize);
}

CRESULT slab_allocate(kmem_slab_t *slab, void **result)
{
    if (!slab)
    {
        *result = NULL;
        return PARAM_ERROR;
    }

    int objId = -1;
    for (int i = 0; i < slab->numBitMapEntry; i++)
    {
        if (slab->pBitmap[i])
        {
            const int addr = i << BITMAP_NUM_BITS_ENTRY_POW_2;
            const int off = FLS(slab->pBitmap[i]) - 1;

            objId = (addr) + off;
            break;
        }
    }

    if (objId == -1)
    {
        *result = NULL;
        return SLAB_FULL;
    }

    *result = (void *)((size_t)slab->memStart + slab->objectSize * objId);
    setBitMap(slab, objId, 0);
    slab->takenSlots++;

    return OK;
}

CRESULT slab_free(kmem_slab_t *slab, const void *ptr)
{
    if (!slab || !ptr)
        return PARAM_ERROR;

    if (slab->memStart > ptr || (size_t)ptr >= (size_t)slab->memStart + slab->slabSize)
        return SLAB_DEALLOC_OBJECT_NOT_IN_SLAB;

    if (((size_t)ptr - (size_t)slab->memStart) % slab->objectSize != 0)
        return SLAB_DEALLOC_NOT_VALID_ADDRES;

    const int id = ((size_t)ptr - (size_t)slab->memStart) / slab->objectSize;

    setBitMap(slab, id, 1);
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

CRESULT slab_find_slab_with_obj(kmem_slab_t *head, const void *ptr, kmem_slab_t **result)
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
        curr = curr->next;
    }
    return FAIL;
}
