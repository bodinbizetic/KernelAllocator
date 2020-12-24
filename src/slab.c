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
        if (!InitializeCriticalSectionAndSpinCount(&s_bufferHead[i].CriticalSection, 0))
        {
            ASSERT(false);
        }
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
    else
    {
        LOG("[KMEMINIT ERROR] %d", code);
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
        LOG("[ERROR] %d %d", code, sizeof(kmem_slab_t));
        PRINT();
        ASSERT(0 && "allocate failed");
        return NULL;
    }

    int x = NUMBER_OF_OBJECTS_IN_SLAB(slab);
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
    void *result = NULL;
    if (pSlab[HAS_SPACE])
    {
        result = slab_allocate_has_space(pSlab, retCode);
    }
    else if (pSlab[EMPTY])
    {
        result = slab_allocate_empty(pSlab, retCode);
    }
    else
    {
        kmem_slab_t *slab;
        CRESULT code = get_slab(objSize, &slab);
        if (code != OK)
        {
            *retCode |= code;
            return NULL;
        }

        slab_list_insert(&pSlab[HAS_SPACE], slab);
        result = slab_allocate_has_space(pSlab, retCode);
    }

    if (constructor)
    {
        constructor(result);
    }

    return result;
}

void *kmalloc(size_t size)
{
    if (!s_bufferHead)
        return NULL;
    const int entryId = BEST_FIT_BLOCKID(size) - 5;
    CRESULT code = OK;
    EnterCriticalSection(&s_bufferHead[entryId].CriticalSection);
    void *ret = slab_allocate_object(s_bufferHead[entryId].pSlab, 1 << BEST_FIT_BLOCKID(size), NULL, &code);
    LeaveCriticalSection(&s_bufferHead[entryId].CriticalSection);
    return ret;
}

static CRESULT slab_kfree_object(kmem_slab_t *pSlab[], void *objp, function destructor)
{
    kmem_slab_t *slab;
    CRESULT code = slab_find_slab_with_obj(pSlab[FULL], objp, &slab);

    if (code == OK)
    {
        if (destructor)
        {
            destructor(objp);
        }
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

        if (destructor)
        {
            destructor(objp);
        }
        code = slab_free(slab, objp);
        slab_list_delete(&pSlab[HAS_SPACE], slab);
    }

    slab_list_insert(&pSlab[(slab->takenSlots ? HAS_SPACE : EMPTY)], slab);
    return OK;
}

void kfree(const void *objp)
{
    if (!objp || !s_bufferHead)
        return;

    for (int i = 0; i < BUFFER_ENTRY_NUM; i++)
    {
        EnterCriticalSection(&s_bufferHead[i].CriticalSection);
        CRESULT code = slab_kfree_object(s_bufferHead[i].pSlab, objp, NULL);
        LeaveCriticalSection(&s_bufferHead[i].CriticalSection);
        if (code == OK)
            return;
    }

    ASSERT(false && "Not Reached");
}

void *kmem_cache_alloc(kmem_cache_t *cachep)
{
    if (!cachep)
        return NULL;

    EnterCriticalSection(&cachep->CriticalSection);
    void *ret = slab_allocate_object(cachep->pSlab, cachep->objectSize, cachep->constructor, &cachep->errorFlags);
    LeaveCriticalSection(&cachep->CriticalSection);
    return ret;
}

void kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
    if (!cachep || !objp)
        return;

    EnterCriticalSection(&cachep->CriticalSection);
    cachep->errorFlags = slab_kfree_object(cachep->pSlab, objp, cachep->destructor);
    LeaveCriticalSection(&cachep->CriticalSection);
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
    cache->pSlab[EMPTY] = NULL;
    cache->pSlab[HAS_SPACE] = NULL;
    cache->pSlab[FULL] = NULL;

    if (!InitializeCriticalSectionAndSpinCount(&cache->CriticalSection, 0x1))
    {
        ASSERT(false);
    }
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size, void (*ctor)(void *), void (*dtor)(void *))
{
    if (!size)
        return NULL;

    EnterCriticalSection(&s_cacheHead->CriticalSection);
    s_cacheHead->errorFlags = OK;
    kmem_cache_t *newCache = kmem_cache_alloc(s_cacheHead);
    if (s_cacheHead->errorFlags != OK)
    {
        LeaveCriticalSection(&s_cacheHead->CriticalSection);
        return NULL;
    }
    kmem_create_cache_init_state(newCache, name, size, ctor, dtor);
    LeaveCriticalSection(&s_cacheHead->CriticalSection);

    return newCache;
}

static int slab_deallocate_list(kmem_slab_t **head)
{
    if (!head)
        return 0;

    int cnt = 0;
    kmem_slab_t *curr = *head;
    while (curr)
    {
        kmem_slab_t *next = curr->next;
        cnt += curr->slabSize / BLOCK_SIZE;
        slab_list_delete(head, curr);
        delete_slab(curr);
        curr = next;
    }
    *head = NULL;

    return cnt;
}

void kmem_cache_destroy(kmem_cache_t *cachep)
{
    if (!cachep)
        return;

    EnterCriticalSection(&cachep->CriticalSection);
    for (enum Slab_Type status = EMPTY; status <= FULL; status++)
    {
        slab_deallocate_list(&cachep->pSlab[status]);
    }
    LeaveCriticalSection(&cachep->CriticalSection);

    DeleteCriticalSection(&cachep->CriticalSection);

    s_cacheHead->errorFlags = OK;

    EnterCriticalSection(&s_cacheHead->CriticalSection);
    kmem_cache_free(s_cacheHead, cachep);
    LeaveCriticalSection(&s_cacheHead->CriticalSection);
}

int kmem_cache_shrink(kmem_cache_t *cachep)
{
    EnterCriticalSection(&cachep->CriticalSection);
    int ret = slab_deallocate_list(&cachep->pSlab[EMPTY]);
    LeaveCriticalSection(&cachep->CriticalSection);
    return ret;
}

void kmem_cache_info(kmem_cache_t *cachep)
{
    int number_slabs = 0;
    int number_blocks = 0;
    int number_objects_free = 0;
    int maxObjects = 0;

    EnterCriticalSection(&cachep->CriticalSection);
    for (enum Slab_Type status = EMPTY; status <= FULL; status++)
        for (kmem_slab_t *curr = cachep->pSlab[status]; curr; curr = curr->next)
        {
            number_slabs++;
            number_blocks += curr->slabSize / BLOCK_SIZE;
            maxObjects += NUMBER_OF_OBJECTS_IN_SLAB(curr);
            for (int i = 0; i < curr->numBitMapEntry; i++)
            {
                int n = 1 << sizeof(BitMapEntry) * CHAR_BIT;
                BitMapEntry num = curr->pBitmap[i];
                while (n)
                {
                    number_objects_free += num & 1;
                    num >>= 1;
                    n >>= 1;
                }
            }
        }
    LeaveCriticalSection(&cachep->CriticalSection);
    printf("Cache info\n");
    printf("Name: %s\n", cachep->name);
    printf("Object size: %llu\n", (unsigned long long)cachep->objectSize);
    printf("Num blocks: %d\n", number_blocks);
    printf("Number slabs: %d\n", number_slabs);
    printf("Number objects: %d\n", maxObjects - number_objects_free);
    printf("Percentage: %f\n", ((double)maxObjects - number_objects_free) / maxObjects);
}
