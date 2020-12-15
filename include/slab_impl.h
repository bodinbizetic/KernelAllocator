#ifndef __slab_impl_H
#define __slab_impl_H

#include "error_codes.h"
#include "helper.h"

#if defined(LOGING) && defined(LOGING_SLAB)

#define SLAB_LOG(...) LOG(__VA_ARGS__)

#else
#define SLAB_LOG(...)
#endif // SLAB_LOG

typedef void (*function)(void *);

typedef uint8_t BitMapEntry;
#define BITMAP_NUM_BITS_ENTRY_POW_2 3
#define NAME_MAX_LEN 32
typedef struct kmem_slab_struct
{
    struct kmem_slab_struct *next;
    struct kmem_slab_struct *prev;
    size_t objectSize;
    size_t slabSize;
    size_t takenSlots;
    BitMapEntry *pBitmap;
    int numBitMapEntry;
    void *memStart;
} kmem_slab_t;

#define NUMBER_OF_OBJECTS_IN_SLAB(slab)                                                                                \
    ((slab->slabSize - sizeof(kmem_slab_t) - slab->numBitMapEntry * sizeof(BitMapEntry)) / slab->objectSize)

enum Slab_Type
{
    EMPTY = 0,
    HAS_SPACE = 1,
    FULL = 2,
    NUM_TYPES = 3
};

struct kmem_cache_struct
{
    struct kmem_cache_struct *next;
    struct kmem_cache_struct *prev;
    size_t objectSize;
    function constructor;
    function destructor;
    CRESULT errorFlags;
    char name[NAME_MAX_LEN];
    kmem_slab_t *pSlab[NUM_TYPES];
};

typedef struct kmem_buffer_struct
{
    size_t size;
    kmem_slab_t *pSlab[NUM_TYPES];
} kmem_buffer_t;

CRESULT get_slab(size_t objectSize, kmem_slab_t **result, function constructor);
CRESULT slab_allocate(kmem_slab_t *slab, void **result);
CRESULT slab_free(kmem_slab_t *slab, const void *ptr);
CRESULT slab_list_insert(kmem_slab_t **head, kmem_slab_t *slab);
CRESULT slab_list_delete(kmem_slab_t **head, kmem_slab_t *slab);
CRESULT slab_find_slab_with_obj(kmem_slab_t *head, const void *ptr, kmem_slab_t **result);

#include "slab.h"

#endif // __slab_impl_H