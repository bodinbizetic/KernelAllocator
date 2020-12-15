#include "buddy/buddy.h"
#include "slab_impl.h"
#include <string.h>

#define BUFFER_SIZE_MAX 17
#define BUFFER_SIZE_MIN 5
#define BUFFER_ENTRY_NUM (BUFFER_SIZE_MAX - BUFFER_SIZE_MIN + 1)

kmem_cache_t *s_cacheHead;
kmem_buffer_t *s_bufferHead;

static void kmem_create_cache_init_state(kmem_cache_t *cache, const char *name, size_t size, void (*ctor)(void *),
                                         void (*dtor)(void *));

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

static void kmem_cache_init()
{
    if (!s_bufferHead)
        return;

    s_cacheHead = (kmem_cache_t *)(s_bufferHead + BUFFER_ENTRY_NUM);
    kmem_create_cache_init_state(s_cacheHead, "\0", sizeof(kmem_cache_t), NULL, NULL);
}

void kmem_init(void *space, int block_num)
{
    ASSERT(BUFFER_SIZE_MAX >= BUFFER_SIZE_MIN);
    s_cacheHead = NULL;

    int code = buddy_init(space, (size_t)block_num * BLOCK_SIZE);
    code |= buddy_alloc(sizeof(kmem_buffer_t) * BUFFER_ENTRY_NUM + sizeof(kmem_cache_t), (void **)&s_bufferHead);

    if (code == OK)
    {
        kmem_buffer_init();
        kmem_cache_init();
    }
}

static void *slab_allocate_has_space(kmem_slab_t *pSlab[], CRESULT *retCode)
{
    if (!pSlab || !pSlab[HAS_SPACE])
    {
        *retCode |= PARAM_ERROR;
        return NULL;
    }

    void *result;
    kmem_slab_t *slab = pSlab[HAS_SPACE];
    CRESULT code = slab_allocate(slab, &result);
    if (code != OK)
    {
        *retCode |= code;
        ASSERT(0 && "allocate failed");
        return NULL;
    }
    if (pSlab[HAS_SPACE]->takenSlots == NUMBER_OF_OBJECTS_IN_SLAB(slab))
    {
        slab_list_delete(&pSlab[HAS_SPACE], slab);
        slab_list_insert(&pSlab[FULL], slab);
    }

    return result;
}

static void *slab_allocate_empty(kmem_slab_t *pSlab[], CRESULT *retCode)
{
    if (!pSlab || !pSlab[EMPTY])
        return NULL;

    if (pSlab[EMPTY])
    {
        kmem_slab_t *slab = pSlab[EMPTY];
        slab_list_delete(&pSlab[EMPTY], slab);
        slab_list_insert(&pSlab[HAS_SPACE], slab);
    }

    return slab_allocate_has_space(pSlab, retCode);
}

static void *slab_allocate_object(kmem_slab_t *pSlab[], size_t objSize, function constructor, CRESULT *retCode)
{
    if (pSlab[HAS_SPACE])
    {
        return slab_allocate_has_space(pSlab, retCode);
    }

    if (pSlab[EMPTY])
    {
        return slab_allocate_empty(pSlab, retCode);
    }
    else
    {
        kmem_slab_t *slab;
        CRESULT code = get_slab(objSize, &slab, constructor);
        if (code != OK)
        {
            *retCode |= code;
            return NULL;
        }

        slab_list_insert(&pSlab[HAS_SPACE], slab);
        return slab_allocate_has_space(pSlab, retCode);
    }

    return NULL;
}

void *kmalloc(size_t size)
{
    const int entryId = BEST_FIT_BLOCKID(size) - 5;
    CRESULT code = OK;
    return slab_allocate_object(s_bufferHead[entryId].pSlab, 1 << BEST_FIT_BLOCKID(size), NULL,
                                &code); // TODO: errorFlags
}

static CRESULT slab_kfree_object(kmem_slab_t *pSlab[], const void *objp)
{
    kmem_slab_t *slab;
    CRESULT code = slab_find_slab_with_obj(pSlab[FULL], objp, &slab);

    if (code == OK)
    {
        code = slab_free(slab, objp);
        ASSERT(code == OK);
        slab_list_delete(&pSlab[FULL], slab);
    }
    else
    {
        code = slab_find_slab_with_obj(pSlab[HAS_SPACE], objp, &slab);
        if (code != OK)
        {
            return FAIL;
        }
        code = slab_free(slab, objp);
        slab_list_delete(&pSlab[HAS_SPACE], slab);
    }

    slab_list_insert(&pSlab[(slab->takenSlots ? HAS_SPACE : EMPTY)], slab);
    return OK;
}

void kfree(const void *objp)
{
    if (!objp)
        return;

    for (int i = 0; i < BUFFER_ENTRY_NUM; i++)
    {
        CRESULT code = slab_kfree_object(s_bufferHead[i].pSlab, objp);
        if (code == OK)
            return;
    }

    ASSERT(false && "Not Reached");
}

void *kmem_cache_alloc(kmem_cache_t *cachep)
{
    if (!cachep)
        return NULL;

    return slab_allocate_object(cachep->pSlab, cachep->objectSize, cachep->constructor, &cachep->errorFlags);
}

void kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
    if (!cachep || !objp)
        return;

    cachep->errorFlags = slab_kfree_object(cachep->pSlab, objp);
}

static void kmem_create_cache_init_state(kmem_cache_t *cache, const char *name, size_t size, void (*ctor)(void *),
                                         void (*dtor)(void *))
{
    if (!cache)
        return;
    cache->constructor = ctor;
    cache->destructor = dtor;
    cache->objectSize = size;
    cache->errorFlags = OK;
    strncpy(cache->name, name, NAME_MAX_LEN - 1);
    cache->next = NULL;
    cache->prev = NULL;
    cache->pSlab[EMPTY] = NULL;
    cache->pSlab[HAS_SPACE] = NULL;
    cache->pSlab[FULL] = NULL;
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size, void (*ctor)(void *), void (*dtor)(void *))
{
    if (!size)
        return NULL;

    s_cacheHead->errorFlags = OK;
    kmem_cache_t *newCache = kmem_cache_alloc(s_cacheHead);
    if (s_cacheHead->errorFlags != OK)
    {
        return NULL;
    }

    kmem_create_cache_init_state(newCache, name, size, ctor, dtor);

    return newCache;
}

static void slab_deallocate_list(kmem_slab_t **head, function destructor)
{
    if (!head)
        return;
    kmem_slab_t *curr = *head;
    while (curr)
    {
        kmem_slab_t *next = curr->next;
        slab_list_delete(head, curr);
        delete_slab(curr, destructor);
        curr = next;
    }
    *head = NULL;
}

void kmem_cache_destroy(kmem_cache_t *cachep)
{
    if (!cachep)
        return NULL;
    s_cacheHead->errorFlags = OK;

    for (enum Slab_Type status = EMPTY; status <= FULL; status++)
    {
        slab_deallocate_list(&cachep->pSlab[status], cachep->destructor);
    }

    kmem_cache_free(s_cacheHead, cachep);
}
