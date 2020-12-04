#include "buddy/buddy.h"
#include "helper.h"

#include <assert.h>
#include <stdint.h>

#define TOTAL_MEMORY_BLOCKID(id) ((1 << id) * ROUND_TO_POWER_OF_TWO(BLOCK_SIZE))

#if POWER_OF_TWO(BLOCK_SIZE)
#define BLOCK_SIZE_POW_TWO BLOCK_SIZE
#else
#define BLOCK_SIZE_POW_TWO ROUND_TO_POWER_OF_TWO(BLOCK_SIZE)
#endif // BLOCK_SIZE_POW_TWO

typedef struct buddy_block_struct
{
    struct buddy_block_struct *prev;
    struct buddy_block_struct *next;
    uint8_t blockid;
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
    for (uint16_t i = 0; i < pBuddyHead->maxBlockSize; i++)
    {
        pBuddyHead->vpMemoryBlocks[i] = NULL;
    }

    buddy_block_t *start = (buddy_block_t *)pBuddyHead->vpMemoryStart;

    for (int16_t i = pBuddyHead->maxBlockSize - 1; i >= 0; i--)
    {
        if ((pBuddyHead->memorySize & TOTAL_MEMORY_BLOCKID(i)) && sizeof(buddy_block_t) <= TOTAL_MEMORY_BLOCKID(i))
        {
            pBuddyHead->vpMemoryBlocks[i] = (buddy_block_t *)start;
            start->next = NULL;
            start->prev = NULL;
            start->blockid = i;
            BUDDY_LOG("Created buddy_block at offset %d of size %d",
                      (intptr_t)start - (intptr_t)pBuddyHead->vpMemoryStart, TOTAL_MEMORY_BLOCKID(i));
            start = (buddy_block_t *)((intptr_t)start + TOTAL_MEMORY_BLOCKID(i)); // / sizeof(buddy_block_t*);
        }
    }
    BUDDY_LOG("Total buddy_blocks allocated: %d", (intptr_t)start - (intptr_t)pBuddyHead->vpMemoryStart);
}

void buddy_init(void *vpSpace, size_t totalSize)
{
    // printf("%d", TOTAL_MEMORY_BLOCKID(0));
    if (!vpSpace || !totalSize) // TODO: Add error
        return;
    LOG("%d", BLOCK_SIZE_POW_TWO);
    const short Num_Blocks = BEST_FIT_BLOCKID(totalSize / BLOCK_SIZE_POW_TWO) + 1;

    if (sizeof(buddy_allocator_t) + Num_Blocks * sizeof(buddy_block_t *) >= totalSize) // TODO: Add error
        return;

    buddy_allocator_t *pBuddyHead = (buddy_allocator_t *)vpSpace;
    pBuddyHead->totalSize = totalSize;
    pBuddyHead->vpStart = vpSpace;
    pBuddyHead->maxBlockSize = Num_Blocks;
    pBuddyHead->vpMemoryBlocks = (buddy_block_t **)((size_t)vpSpace + sizeof(buddy_allocator_t));
    pBuddyHead->vpMemoryStart = pBuddyHead->vpMemoryBlocks + sizeof(buddy_block_t *) * pBuddyHead->maxBlockSize;
    pBuddyHead->memorySize = totalSize - sizeof(buddy_allocator_t) - sizeof(buddy_block_t *) * pBuddyHead->maxBlockSize;
    s_pBuddyHead = pBuddyHead;

    BUDDY_LOG("Size of buddy: %d\nSize of array: %d", sizeof(buddy_allocator_t), Num_Blocks * sizeof(buddy_block_t *));

    buddy_init_memory_blocks(pBuddyHead);
    buddy_print_memory_offsets();
}

static void buddy_remove_from_current_list(buddy_block_t *toRemove)
{
    if (!toRemove)
        return;

    if (!toRemove->prev)
    {
        ASSERT(toRemove->blockid < s_pBuddyHead->maxBlockSize);
        ASSERT(s_pBuddyHead->vpMemoryBlocks[toRemove->blockid] == toRemove);
        s_pBuddyHead->vpMemoryBlocks[toRemove->blockid] = toRemove->next;
        if (toRemove->next)
        {
            toRemove->next->prev = NULL;
        }
    }
    else
    {
        toRemove->prev->next = toRemove->next;
        if (toRemove->next)
        {
            toRemove->next->prev = toRemove->prev;
        }
    }
    toRemove->next = NULL;
    toRemove->prev = NULL;
}

static void buddy_insert_block(buddy_block_t *toInsert)
{
    if (!toInsert)
        return;

    ASSERT(toInsert->blockid < s_pBuddyHead->maxBlockSize);
    buddy_block_t *curr = s_pBuddyHead->vpMemoryBlocks[toInsert->blockid];
    buddy_block_t *prev = NULL;

    while (curr)
    {
        if (curr > toInsert)
            break;
        prev = curr;
        curr = curr->next;
    }

    if (prev)
    {
        toInsert->next = prev->next;
        toInsert->prev = prev;
        if (prev->next)
        {
            prev->next->prev = toInsert;
        }
        prev->next = toInsert;
    }
    else
    {
        toInsert->next = s_pBuddyHead->vpMemoryBlocks[toInsert->blockid];
        toInsert->prev = NULL;
        if (s_pBuddyHead->vpMemoryBlocks[toInsert->blockid])
        {
            s_pBuddyHead->vpMemoryBlocks[toInsert->blockid]->prev = toInsert;
        }
        s_pBuddyHead->vpMemoryBlocks[toInsert->blockid] = toInsert;
    }
}

static void buddy_split_once(buddy_block_t *toSplit, bool toRemove)
{
    if (!toSplit || !toSplit->blockid)
        return;

    ASSERT(sizeof(buddy_block_t) <= TOTAL_MEMORY_BLOCKID(toSplit->blockid - 1));
    if (toRemove)
    {
        buddy_remove_from_current_list(toSplit);
    }

    buddy_block_t *newBuddy = (buddy_block_t *)((intptr_t)toSplit + TOTAL_MEMORY_BLOCKID(toSplit->blockid - 1));
    newBuddy->prev = NULL;
    newBuddy->next = NULL;
    newBuddy->blockid = toSplit->blockid - 1;
    toSplit->blockid--;

    buddy_insert_block(newBuddy);
}

static void *buddy_split_buddy_block(uint8_t blockid, uint8_t targetBlockid)
{
    ASSERT(blockid > targetBlockid);
    ASSERT(blockid < s_pBuddyHead->maxBlockSize);

    buddy_block_t *toSplitStart = s_pBuddyHead->vpMemoryBlocks[blockid];
    if (!toSplitStart) // TODO: Add error
        return NULL;

    for (uint8_t i = blockid; i != targetBlockid; i--)
    {
        buddy_split_once(toSplitStart, i == blockid);
    }

    return toSplitStart;
}

void *buddy_alloc(size_t size)
{
    if (!size) // TODO: Add error
        return NULL;

    const size_t numBlocks = (size + BLOCK_SIZE_POW_TWO - 1) / BLOCK_SIZE_POW_TWO;

    if (numBlocks >= s_pBuddyHead->maxBlockSize) // TODO: Add error
        return NULL;

    void *result = NULL;
    if (s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)])
    {
        result = s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)];
        s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)] =
            s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)]->next;
    }
    else
    {
        uint8_t i;
        for (i = BEST_FIT_BLOCKID(numBlocks) + 1; i < s_pBuddyHead->maxBlockSize; i++)
        {
            if (s_pBuddyHead->vpMemoryBlocks[i])
                break;
        }
        if (i == s_pBuddyHead->maxBlockSize) // TODO: Add error no memory
            return NULL;

        result = buddy_split_buddy_block(i, BEST_FIT_BLOCKID(numBlocks));
    }

    return result;
}

void buddy_print_memory_offsets()
{
    printf("//*************//\n");
    for (uint8_t i = 0; i < s_pBuddyHead->maxBlockSize; i++)
    {
        printf("[ %4d ] ", 1 << i);
        for (buddy_block_t *curr = s_pBuddyHead->vpMemoryBlocks[i]; curr; curr = curr->next)
            printf(" %d", (intptr_t)curr - (intptr_t)s_pBuddyHead->vpMemoryStart);
        printf("\n");
    }
}
