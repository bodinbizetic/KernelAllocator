#include <stdint.h>
#include "buddy/buddy.h"
#include "helper.h"

#define TOTAL_MEMORY_BLOCKID(id) (1 << id)

typedef struct buddy_block
{
    void* next;
} buddy_block_t;

typedef struct buddy_allocator
{
    void *vpStart;
    size_t totalSize;
    uint8_t maxBlockSize;
    void *vpMemoryStart;
    size_t memorySize;
    buddy_block_t **vpMemoryBlocks;
} buddy_allocator_t;

static uint8_t get_max_block_size(size_t totalSize)
{
    uint8_t blockCnt = 0;
    while(totalSize)
    {
        totalSize >>= 1;
        blockCnt++;
    }

    return blockCnt;
}

static void buddy_init_memory_blocks(buddy_allocator_t *pBuddyHead)
{
    for (uint16_t i=0; i < pBuddyHead->maxBlockSize; i++)
    {
        pBuddyHead->vpMemoryBlocks[i] = NULL;
    }

    buddy_block_t* start = (buddy_block_t*)pBuddyHead->vpMemoryStart;
    for (int16_t i=pBuddyHead->maxBlockSize-1; i >= 0; i--)
    {
        if (pBuddyHead->memorySize & TOTAL_MEMORY_BLOCKID(i) && sizeof(buddy_block_t) <= TOTAL_MEMORY_BLOCKID(i))
        {
            pBuddyHead->vpMemoryBlocks[i] = (buddy_block_t*) start;
            start->next = NULL;
            LOG("Created buddy_block at offset %d of size %d", (size_t)start - (size_t)pBuddyHead->vpMemoryStart, TOTAL_MEMORY_BLOCKID(i));
            start += TOTAL_MEMORY_BLOCKID(i);
        }
    }
}

void buddy_init(void* vpSpace, size_t totalSize)
{
    if (!vpSpace) // TODO: Add error
        return;

    const short Num_Blocks = get_max_block_size(totalSize);

    if (sizeof(buddy_allocator_t) + Num_Blocks * sizeof(buddy_block_t*) >= totalSize) // TODO: Add error
        return;

    buddy_allocator_t *pBuddyHead = (buddy_allocator_t*) vpSpace;
    pBuddyHead->totalSize       = totalSize;
    pBuddyHead->vpStart         = vpSpace;
    pBuddyHead->maxBlockSize    = Num_Blocks;
    pBuddyHead->vpMemoryBlocks  = (buddy_block_t**)((size_t)vpSpace + sizeof(buddy_allocator_t));
    pBuddyHead->vpMemoryStart   = pBuddyHead->vpMemoryBlocks + sizeof(buddy_block_t*) * pBuddyHead->maxBlockSize;
    pBuddyHead->memorySize      = totalSize - sizeof(buddy_allocator_t) - sizeof(buddy_block_t*) * pBuddyHead->maxBlockSize;

    LOG("Size of buddy: %d", sizeof(buddy_allocator_t));

    buddy_init_memory_blocks(pBuddyHead);
}