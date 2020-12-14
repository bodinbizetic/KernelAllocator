#include "buddy/buddy.h"
#include "error_codes.h"
#include "helper.h"
#include "tests.h"

extern buddy_allocator_t *s_pBuddyHead;
void *unusedPointer;
BUDDY_TEST_START(full_range_memory)
{
    for (int i = 0; i < num_blocks; i++)
    {
        tst_OK(buddy_alloc(BUDDY_BLOCK_SIZE, &unusedPointer));
    }
    tst_FAIL(buddy_alloc(BUDDY_BLOCK_SIZE, &unusedPointer));
}
BUDDY_TEST_END

BUDDY_TEST_START(out_of_mem)
{
    tst_OK(buddy_alloc((num_blocks + 1) * BUDDY_BLOCK_SIZE, &unusedPointer) & ~NOT_ENOUGH_MEMORY);
}
BUDDY_TEST_END

BUDDY_TEST_START(alloc_free)
{
    const int ITER = 1000;
    const int mem = (ROUND_TO_POWER_OF_TWO(num_blocks) >> 1) * BUDDY_BLOCK_SIZE;
    for (int i = 0; i < ITER; i++)
    {
        if (mem == 0)
            return true;
        void *ptr;
        tst_OK(buddy_alloc(mem, &ptr));
        tst_assert(ptr);
        buddy_free(ptr, mem);
    }

    size_t check_sum = 0;
    for (int i = 0; i < BEST_FIT_BLOCKID(num_blocks) + 1; i++)
    {
        check_sum += ((s_pBuddyHead->vpMemoryBlocks[i].block != NULL) << i);
        tst_assert(!s_pBuddyHead->vpMemoryBlocks[i].block ||
                   !s_pBuddyHead->vpMemoryBlocks[i].block->next && "Has Only One in Entry");
    }
    tst_assert(check_sum == num_blocks);
}
BUDDY_TEST_END

BUDDY_TEST_START(full_alloc_free)
{
    void **ptr = malloc(num_blocks * sizeof(void *));
    for (int i = 0; i < num_blocks; i++)
    {
        tst_OK(buddy_alloc(BUDDY_BLOCK_SIZE, &ptr[i]));
    }
    tst_FAIL(buddy_alloc(BUDDY_BLOCK_SIZE, ptr));

    for (int i = 0; i < num_blocks; i++)
    {
        tst_OK(buddy_free(ptr[i], BUDDY_BLOCK_SIZE));
    }
    size_t check_sum = 0;
    for (int i = 0; i < BEST_FIT_BLOCKID(num_blocks) + 1; i++)
    {
        check_sum += ((s_pBuddyHead->vpMemoryBlocks[i].block != NULL) << i);
        tst_assert(!s_pBuddyHead->vpMemoryBlocks[i].block ||
                   !s_pBuddyHead->vpMemoryBlocks[i].block->next && "Has Only One in Entry");
    }
    tst_assert(check_sum == num_blocks);
}
BUDDY_TEST_END

BUDDY_TEST_START(full_alloc_free_32)
{
    const int size = 32;
    void **ptr = malloc(num_blocks * sizeof(void *));
    for (int i = 0; i < num_blocks / 32; i++)
    {
        tst_OK(buddy_alloc(size * BUDDY_BLOCK_SIZE, &ptr[i]));
    }
    tst_assert(buddy_alloc(size * BUDDY_BLOCK_SIZE, ptr));

    for (int i = 0; i < num_blocks / 32; i++)
    {
        tst_OK(buddy_free(ptr[i], size * BUDDY_BLOCK_SIZE));
    }
    size_t check_sum = 0;
    for (int i = 0; i < BEST_FIT_BLOCKID(num_blocks) + 1; i++)
    {
        check_sum += ((s_pBuddyHead->vpMemoryBlocks[i].block != NULL) << i);
        tst_assert(!s_pBuddyHead->vpMemoryBlocks[i].block ||
                   !s_pBuddyHead->vpMemoryBlocks[i].block->next && "Has Only One in Entry");
    }
    tst_assert(check_sum == num_blocks);
}
BUDDY_TEST_END

TEST_SUITE_START(buddy, 1024)
{
    SUITE_ADD(full_range_memory);
    SUITE_ADD(out_of_mem);
    SUITE_ADD(alloc_free);
    SUITE_ADD(full_alloc_free);
    SUITE_ADD(full_alloc_free_32);
}
TEST_SUITE_END

TEST_SUITE_START(buddy_2, 3)
{
    SUITE_ADD(full_range_memory);
    SUITE_ADD(out_of_mem);
    SUITE_ADD(alloc_free);
    SUITE_ADD(full_alloc_free);
    SUITE_ADD(full_alloc_free_32);
}
TEST_SUITE_END
