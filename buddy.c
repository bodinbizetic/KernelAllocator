#include <stdint.h>
#include <assert.h>
#include "buddy/buddy.h"
#include "helper.h"

#define TOTAL_MEMORY_BLOCKID(id) ((1 << id) * ROUND_TO_POWER_OF_TWO(BLOCK_SIZE))

typedef struct buddy_block_struct
{
    struct buddy_block_struct *prev;
    struct buddy_block_struct *next;
} buddy_block_t;

typedef struct buddy_allocator_struct
{
    void *vpStart;
    size_t totalSize;
    uint8_t maxBlockSize;
    void *vpMemoryStart;
    size_t memorySize;
    buddy_block_t **vpMemoryBlocks;
} buddy_allocator_t;

static buddy_allocator_t *s_pBuddyHead;

static inline void buddy_init_memory_blocks(buddy_allocator_t *pBuddyHead)
{
    //assert(POWER_OF_TWO(BLOCK_SIZE));

    for (uint16_t i=0; i < pBuddyHead->maxBlockSize; i++)
    {
        pBuddyHead->vpMemoryBlocks[i] = NULL;
    }

    buddy_block_t* start = (buddy_block_t*)pBuddyHead->vpMemoryStart;
    const uint16_t Block_Size_Fls = BEST_FIT_BLOCKID(BLOCK_SIZE)-1;
    for (int16_t i=pBuddyHead->maxBlockSize-1; i >= 0; i--)
    {
        if ((pBuddyHead->memorySize & TOTAL_MEMORY_BLOCKID(i))
                && sizeof(buddy_block_t) <= TOTAL_MEMORY_BLOCKID(i))
        {
            pBuddyHead->vpMemoryBlocks[i] = (buddy_block_t*) start;
            start->next = NULL;
            start->prev = NULL;
            BUDDY_LOG("Created buddy_block at offset %d of size %d", (size_t)start - (size_t)pBuddyHead->vpMemoryStart, TOTAL_MEMORY_BLOCKID(i));
            start += TOTAL_MEMORY_BLOCKID(i) / sizeof(buddy_block_t);
        }
    }
    BUDDY_LOG("Total buddy_blocks allocated: %d", (size_t)start - (size_t)pBuddyHead->vpMemoryStart);
}

void buddy_init(void* vpSpace, size_t totalSize)
{
    //assert(POWER_OF_TWO(BLOCK_SIZE));

    if (!vpSpace || !totalSize) // TODO: Add error
        return;

    const short Num_Blocks = BEST_FIT_BLOCKID(totalSize);

    if (sizeof(buddy_allocator_t) + Num_Blocks * sizeof(buddy_block_t*) >= totalSize) // TODO: Add error
        return;

    buddy_allocator_t *pBuddyHead = (buddy_allocator_t*) vpSpace;
    pBuddyHead->totalSize       = totalSize;
    pBuddyHead->vpStart         = vpSpace;
    pBuddyHead->maxBlockSize    = Num_Blocks;
    pBuddyHead->vpMemoryBlocks  = (buddy_block_t**)((size_t)vpSpace + sizeof(buddy_allocator_t));
    pBuddyHead->vpMemoryStart   = pBuddyHead->vpMemoryBlocks + sizeof(buddy_block_t*) * pBuddyHead->maxBlockSize;
    pBuddyHead->memorySize      = totalSize - sizeof(buddy_allocator_t) - sizeof(buddy_block_t*) * pBuddyHead->maxBlockSize;
    s_pBuddyHead = pBuddyHead;

    BUDDY_LOG("Size of buddy: %d\nSize of array: %d", sizeof(buddy_allocator_t), Num_Blocks * sizeof(buddy_block_t*));

    buddy_init_memory_blocks(pBuddyHead);
}
