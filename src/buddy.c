#include "buddy/buddy.h"
#include "helper.h"

#include <assert.h>

#define TOTAL_MEMORY_BLOCKID(id) ((1 << id) * ROUND_TO_POWER_OF_TWO(BUDDY_BLOCK_SIZE))

#if POWER_OF_TWO(BUDDY_BLOCK_SIZE)
#define BLOCK_SIZE_POW_TWO BUDDY_BLOCK_SIZE
#else
#define BLOCK_SIZE_POW_TWO ROUND_TO_POWER_OF_TWO(BUDDY_BLOCK_SIZE)
#endif // BLOCK_SIZE_POW_TWO

#define LEFT_RIGHT_BROTHER(ptrSmall, ptrBig, blockId)                                                                  \
    ((intptr_t)ptrSmall + TOTAL_MEMORY_BLOCKID(blockId) == (intptr_t)ptrBig)

#define BUDDY_BITMAP_MIN_BLOCKID 0

#define BIT_ID_ADR(addr, blockid)                                                                                      \
    (((size_t)addr - (size_t)s_pBuddyHead->vpMemoryStart) / BLOCK_SIZE_POW_TWO >> blockid + 4)
#define BIT_ID_OFF(addr, blockid)                                                                                      \
    (1 << ((((size_t)addr - (size_t)s_pBuddyHead->vpMemoryStart) / BLOCK_SIZE_POW_TWO >> blockid + 1) & 0x7))

#define YOUNG_BROTHER(x, y) (x < y ? x : y)
#define OLD_BROTHER(x, y) (x > y ? x : y)

buddy_allocator_t *s_pBuddyHead = NULL;

static inline void setBitMapBit(void *addr, uint8_t blockId, uint8_t value)
{
    ASSERT(addr >= s_pBuddyHead->vpMemoryStart);
    if (s_pBuddyHead->vpMemoryBlocks[blockId].numByteMap)
    {
        const size_t indexAdr = BIT_ID_ADR(addr, blockId);
        const uint8_t bitOff = BIT_ID_OFF(addr, blockId);
        const uint8_t byteMap = (value ? s_pBuddyHead->vpMemoryBlocks[blockId].bitmap[indexAdr] | bitOff
                                       : s_pBuddyHead->vpMemoryBlocks[blockId].bitmap[indexAdr] & ~bitOff);
        s_pBuddyHead->vpMemoryBlocks[blockId].bitmap[BIT_ID_ADR(addr, blockId)] = byteMap;
    }
}

static inline bool getBitMapBit(void *addr, uint8_t blockId)
{
    ASSERT(addr);
    if (s_pBuddyHead->vpMemoryBlocks[blockId].numByteMap)
    {
        const size_t indexAdr = BIT_ID_ADR(addr, blockId);
        const uint8_t bitOff = BIT_ID_OFF(addr, blockId);
        return (s_pBuddyHead->vpMemoryBlocks[blockId].bitmap[indexAdr] & bitOff) != 0;
    }
}

static inline buddy_block_t *getBrother(buddy_block_t *pBlock)
{
    intptr_t adr = (intptr_t)pBlock - (intptr_t)s_pBuddyHead->vpMemoryStart;
    adr ^= BUDDY_BLOCK_SIZE << pBlock->blockid;
    return adr + (intptr_t)s_pBuddyHead->vpMemoryStart;
}

static inline void buddy_init_memory_blocks(buddy_allocator_t *pBuddyHead)
{
    for (uint16_t i = 0; i < pBuddyHead->maxBlockSize; i++)
    {
        pBuddyHead->vpMemoryBlocks[i].block = NULL;
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

            setBitMapBit(start, i, 1);

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
    size_t memory = pBuddyHead->memorySize / BLOCK_SIZE_POW_TWO;
    size_t memory_loss = 0;
    for (uint8_t i = 0; i < pBuddyHead->maxBlockSize; i++)
    {
        if (i < BUDDY_BITMAP_MIN_BLOCKID)
        {
            pBuddyHead->vpMemoryBlocks[i].bitmap = NULL;
            pBuddyHead->vpMemoryBlocks[i].numByteMap = 0;
        }
        else
        {
            pBuddyHead->vpMemoryBlocks[i].bitmap = (uint8_t *)start;
            size_t offset = memory >> 4 + i;
            offset++;
            pBuddyHead->vpMemoryBlocks[i].numByteMap = offset;
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

    ASSERT(blockid >= BUDDY_BITMAP_MIN_BLOCKID && getBitMapBit(toSplitStart, blockid) == 1);
    for (uint8_t i = blockid; i != targetBlockid; i--)
    {
        buddy_split_once(toSplitStart, i == blockid);
        setBitMapBit(toSplitStart, i, getBitMapBit(toSplitStart, i) ^ 1);
    }

    setBitMapBit(toSplitStart, targetBlockid, 1);
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
        ASSERT(
            BEST_FIT_BLOCKID(numBlocks) >= BUDDY_BITMAP_MIN_BLOCKID &&
            getBitMapBit(s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block, BEST_FIT_BLOCKID(numBlocks)));

        result = s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block;
        s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block =
            s_pBuddyHead->vpMemoryBlocks[BEST_FIT_BLOCKID(numBlocks)].block->next;

        setBitMapBit(result, BEST_FIT_BLOCKID(numBlocks), 0);
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

static void buddy_merge_propagate(buddy_block_t *pBuddyBlock)
{
    if (!pBuddyBlock)
        return;

    while (true)
    {
        if (getBitMapBit(pBuddyBlock, pBuddyBlock->blockid))
        {
            buddy_block_t *brother = getBrother(pBuddyBlock);
            buddy_remove_from_current_list(brother);
            pBuddyBlock = YOUNG_BROTHER(pBuddyBlock, brother);
            setBitMapBit(pBuddyBlock, pBuddyBlock->blockid, 0);
            pBuddyBlock->blockid++;
        }
        else
        {
            setBitMapBit(pBuddyBlock, pBuddyBlock->blockid, 1);
            buddy_insert_block(pBuddyBlock);
            break;
        }
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
    buddy_merge_propagate(pBuddyBlock);
}

void buddy_print_memory_offsets()
{
    printf("//*************//\n");
    for (uint8_t i = 0; i < s_pBuddyHead->maxBlockSize; i++)
    {
        printf("[ %4d ] ", 1 << i);
        for (buddy_block_t *curr = s_pBuddyHead->vpMemoryBlocks[i].block; curr; curr = curr->next)
            printf(" (%d, %d)", (intptr_t)curr - (intptr_t)s_pBuddyHead->vpMemoryStart,
                   ((intptr_t)curr - (intptr_t)s_pBuddyHead->vpMemoryStart) / BLOCK_SIZE_POW_TWO >> curr->blockid);
        printf("\n");
    }
}

void buddy_print_bitmap()
{
    printf("//*************//\n");
    for (uint8_t i = 0; i < s_pBuddyHead->maxBlockSize; i++)
    {
        printf("[ %4d ] ", 1 << i);
        if (s_pBuddyHead->vpMemoryBlocks[i].numByteMap == 0)
        {
            printf("NULL\n");
            continue;
        }
        for (size_t j = 0; j < s_pBuddyHead->vpMemoryBlocks[i].numByteMap; j++)
        {

            uint8_t n = s_pBuddyHead->vpMemoryBlocks[i].bitmap[j];
            uint8_t cnt = 8;
            while (cnt)
            {
                if (n & 1)
                    printf("1");
                else
                    printf("0");

                n >>= 1;
                cnt--;
            }
            printf(" ");
        }
        printf("\n");
    }
}
