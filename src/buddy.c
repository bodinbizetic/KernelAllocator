#include "buddy/buddy.h"
#include "helper.h"

#include <assert.h>

#define TOTAL_MEMORY_BLOCKID(id) ((1 << id) * ROUND_TO_POWER_OF_TWO(BUDDY_BLOCK_SIZE))

#if POWER_OF_TWO(BLOCK_SIZE)
#define BLOCK_SIZE_POW_TWO BUDDY_BLOCK_SIZE
#else
#define BLOCK_SIZE_POW_TWO ROUND_TO_POWER_OF_TWO(BUDDY_BLOCK_SIZE)
#endif // BLOCK_SIZE_POW_TWO

#define LEFT_RIGHT_BROTHER(ptrSmall, ptrBig, blockId)                                                                  \
    ((intptr_t)ptrSmall + TOTAL_MEMORY_BLOCKID(blockId) == (intptr_t)ptrBig)

#define BUDDY_BITMAP_MIN_BLOCKID 4

buddy_allocator_t *s_pBuddyHead = NULL;

static inline void buddy_init_memory_blocks(buddy_allocator_t *pBuddyHead)
{
    for (uint16_t i = 0; i < pBuddyHead->maxBlockSize; i++)
    {
        pBuddyHead->vpMemoryBlocks[i].block = NULL;
        pBuddyHead->vpMemoryBlocks[i].bitmap = NULL;
    }

    buddy_block_t *start = (buddy_block_t *)pBuddyHead->vpMemoryStart;

    for (int16_t i = pBuddyHead->maxBlockSize - 1; i >= 0; i--)
    {
        if ((pBuddyHead->memorySize & TOTAL_MEMORY_BLOCKID(i)) && sizeof(buddy_block_t) <= TOTAL_MEMORY_BLOCKID(i))
        {
            pBuddyHead->vpMemoryBlocks[i].block = (buddy_block_t *)start;
            start->next = NULL;
            start->prev = NULL;
            start->blockid = i;
            BUDDY_LOG("Created buddy_block at offset %d of size %d",
                      (intptr_t)start - (intptr_t)pBuddyHead->vpMemoryStart, TOTAL_MEMORY_BLOCKID(i));
            start = (buddy_block_t *)((intptr_t)start + TOTAL_MEMORY_BLOCKID(i));
        }
    }
    BUDDY_LOG("Total buddy_blocks allocated: %d", (intptr_t)start - (intptr_t)pBuddyHead->vpMemoryStart);
}

static void buddy_init_bitmap(buddy_allocator_t *pBuddyHead)
{
    if (!pBuddyHead)
        return;
    void *start = pBuddyHead->vpMemoryStart;
    size_t memory = pBuddyHead->memorySize;
    size_t memory_loss = 0;
    for (uint8_t i = 0; i < pBuddyHead->maxBlockSize; i++)
    {
        if (i < BUDDY_BITMAP_MIN_BLOCKID)
        {
            pBuddyHead->vpMemoryBlocks[i].bitmap = NULL;
        }
        else
        {
            pBuddyHead->vpMemoryBlocks[i].bitmap = (uint8_t *)start;
            size_t offset = memory >> 4 + i;
            offset++;
            memory_loss += offset;
            for (size_t ii = 0; ii < offset; ii++)
            {
                pBuddyHead->vpMemoryBlocks[i].bitmap[ii] = 0;
            }
            start = (intptr_t)start + offset;
        }
    }
    pBuddyHead->vpMemoryStart = start;
    pBuddyHead->memorySize -= memory_loss;
    BUDDY_LOG("BITMAP loss: %ld", memory_loss);
    BUDDY_LOG("Memory start at index: %ld", (intptr_t)start - (intptr_t)pBuddyHead->vpStart);
}

void buddy_init(void *vpSpace, size_t totalSize)
{
    if (!vpSpace || !totalSize) // TODO: Add error
        return;

    const short Num_Blocks = BEST_FIT_BLOCKID(totalSize / BLOCK_SIZE_POW_TWO) + 1;

    if (sizeof(buddy_allocator_t) + Num_Blocks * sizeof(buddy_block_t *) >= totalSize) // TODO: Add error
        return;

    buddy_allocator_t *pBuddyHead = (buddy_allocator_t *)vpSpace;
    pBuddyHead->totalSize = totalSize;
    pBuddyHead->vpStart = vpSpace;
    pBuddyHead->maxBlockSize = Num_Blocks;
    pBuddyHead->vpMemoryBlocks = (buddy_table_entry_t *)((size_t)vpSpace + sizeof(buddy_allocator_t));
    pBuddyHead->vpMemoryStart = (void *)((size_t)pBuddyHead->vpMemoryBlocks + sizeof(buddy_table_entry_t) * Num_Blocks);
    pBuddyHead->memorySize = totalSize - sizeof(buddy_allocator_t) - sizeof(buddy_table_entry_t) * Num_Blocks;
    s_pBuddyHead = pBuddyHead;

    BUDDY_LOG("Size of buddy: %d\nSize of array: %d", sizeof(buddy_allocator_t), Num_Blocks * sizeof(buddy_block_t *));

    buddy_init_bitmap(pBuddyHead);
    buddy_init_memory_blocks(pBuddyHead);
}

static void buddy_remove_from_current_list(buddy_block_t *toRemove)
{
    if (!toRemove)
        return;

    if (!toRemove->prev)
    {
        ASSERT(toRemove->blockid < s_pBuddyHead->maxBlockSize);
        ASSERT(s_pBuddyHead->vpMemoryBlocks[toRemove->blockid].block == toRemove);
        s_pBuddyHead->vpMemoryBlocks[toRemove->blockid].block = toRemove->next;
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
    buddy_block_t *curr = s_pBuddyHead->vpMemoryBlocks[toInsert->blockid].block;
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
        toInsert->next = s_pBuddyHead->vpMemoryBlocks[toInsert->blockid].block;
        toInsert->prev = NULL;
        if (s_pBuddyHead->vpMemoryBlocks[toInsert->blockid].block)
        {
            s_pBuddyHead->vpMemoryBlocks[toInsert->blockid].block->prev = toInsert;
        }
        s_pBuddyHead->vpMemoryBlocks[toInsert->blockid].block = toInsert;
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

    buddy_block_t *toSplitStart = s_pBuddyHead->vpMemoryBlocks[blockid].block;
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
    if (s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block)
    {
        result = s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block;
        s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block =
            s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block->next;
    }
    else
    {
        uint8_t i;
        for (i = BEST_FIT_BLOCKID(numBlocks) + 1; i < s_pBuddyHead->maxBlockSize; i++)
        {
            if (s_pBuddyHead->vpMemoryBlocks[i].block)
                break;
        }
        if (i == s_pBuddyHead->maxBlockSize) // TODO: Add error no memory
            return NULL;

        result = buddy_split_buddy_block(i, BEST_FIT_BLOCKID(numBlocks));
    }

    return result;
}

static void buddy_merge_big_level_up(buddy_block_t *pBuddyBlock)
{
    if (!pBuddyBlock)
        return;
    ASSERT(pBuddyBlock->next);

    buddy_remove_from_current_list(pBuddyBlock->next);
    buddy_remove_from_current_list(pBuddyBlock);

    pBuddyBlock->blockid++;
    ASSERT(pBuddyBlock->blockid < s_pBuddyHead->maxBlockSize);
    buddy_insert_block(pBuddyBlock);
}

static void buddy_merge_propagate(buddy_block_t *pBuddyBlock)
{
    if (!pBuddyBlock)
        return;

    while (true)
    {
        buddy_block_t *toMerge = NULL;
        if (pBuddyBlock->next && LEFT_RIGHT_BROTHER(pBuddyBlock, pBuddyBlock->next, pBuddyBlock->blockid))
        {
            toMerge = pBuddyBlock;
        }
        if (pBuddyBlock->prev && LEFT_RIGHT_BROTHER(pBuddyBlock->prev, pBuddyBlock, pBuddyBlock->blockid))
        {
            toMerge = pBuddyBlock->prev;
            pBuddyBlock = toMerge;
        }
        if (!toMerge)
            break;
        buddy_merge_big_level_up(toMerge);
    }
}

void buddy_free(void *ptr, size_t size)
{
    if (!ptr || !size)
        return; // TODO: Add Error

    const size_t numBlocks = (size + BLOCK_SIZE_POW_TWO - 1) / BLOCK_SIZE_POW_TWO;
    buddy_block_t *const pBuddyBlock = ptr;
    pBuddyBlock->blockid = BEST_FIT_BLOCKID(size / BLOCK_SIZE_POW_TWO);
    pBuddyBlock->next = NULL;
    pBuddyBlock->prev = NULL;
    buddy_insert_block(ptr);
    buddy_merge_propagate(pBuddyBlock);
}

void buddy_print_memory_offsets()
{
    printf("//*************//\n");
    for (uint8_t i = 0; i < s_pBuddyHead->maxBlockSize; i++)
    {
        printf("[ %4d ] ", 1 << i);
        for (buddy_block_t *curr = s_pBuddyHead->vpMemoryBlocks[i].block; curr; curr = curr->next)
            printf(" %d", (intptr_t)curr - (intptr_t)s_pBuddyHead->vpMemoryStart);
        printf("\n");
    }
}
