#include "slab.h"
#include "buddy/buddy.h"
#include "error_codes.h"

#include "slab_impl.h"

#define BUFFER_SIZE_MAX 17
#define BUFFER_SIZE_MIN 5
#define BUFFER_ENTRY_NUM (BUFFER_SIZE_MAX - BUFFER_SIZE_MIN + 1)

static kmem_cache_t *s_cacheHead;
static kmem_buffer_t *s_bufferHead;

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