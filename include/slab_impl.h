#ifndef __slab_impl_H
#define __slab_impl_H

#include "helper.h"
#include "slab.h"

#if defined(LOGING) && defined(LOGING_SLAB)

#define SLAB_LOG(...) LOG(__VA_ARGS__)

#else
#define SLAB_LOG(...)
#endif // SLAB_LOG

typedef void (*function)(void *);

typedef struct kmem_slab_struct
{
    struct kmem_slab_struct *next;
    struct kmem_slab_struct *prev;
    size_t objectSize;
    size_t slabSize;
    size_t freeSlots;
    void *free;
} kmem_slab_t;

struct kmem_cache_struct
{
    struct kmem_cache_struct *next;
    struct kmem_cache_struct *prev;
    size_t objectSize;
    function constructor;
    function destructor;
    const char *name;
    kmem_slab_t *pSlabEmpty;
    kmem_slab_t *pSlabFull;
    kmem_slab_t *pSlabHasSpace;
};

typedef struct kmem_buffer_struct
{
    size_t size;
    kmem_slab_t *pSlabEmpty;
    kmem_slab_t *pSlabFull;
    kmem_slab_t *pSlabHasSpace;
} kmem_buffer_t;

int get_slab(size_t objectSize, kmem_slab_t **result);
int slab_allocate(kmem_slab_t *slab, void **result);

#endif // __slab_impl_H