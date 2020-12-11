#include "buddy/buddy.h"
#include "helper.h"
#include "tests.h"

extern buddy_allocator_t *s_pBuddyHead;

BUDDY_TEST_START(full_range_memory)
{
    buddy_print_memory_offsets();
    for (int i = 0; i < num_blocks; i++)
    {
        tst_assert(buddy_alloc(BUDDY_BLOCK_SIZE));
    }
    tst_assert(!buddy_alloc(BUDDY_BLOCK_SIZE));
    buddy_print_memory_offsets();
}
BUDDY_TEST_END

BUDDY_TEST_START(out_of_mem)
{
    tst_assert(!buddy_alloc((num_blocks + 1) * BUDDY_BLOCK_SIZE));
}
BUDDY_TEST_END

BUDDY_TEST_START(alloc_free)
{
    const int ITER = 1000;
    const int mem = num_blocks / 8 * BUDDY_BLOCK_SIZE;
    for (int i = 0; i < ITER; i++)
    {
        void *ptr = buddy_alloc(mem);
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
    buddy_print_memory_offsets();
    for (int i = 0; i < num_blocks; i++)
    {
        tst_assert(ptr[i] = buddy_alloc(BUDDY_BLOCK_SIZE));
    }
    tst_assert(!buddy_alloc(BUDDY_BLOCK_SIZE));

    for (int i = 0; i < num_blocks; i++)
    {
        buddy_free(ptr[i], BUDDY_BLOCK_SIZE);
    }
    buddy_print_memory_offsets();
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
    buddy_print_memory_offsets();
    for (int i = 0; i < num_blocks / 32; i++)
    {
        tst_assert(ptr[i] = buddy_alloc(size * BUDDY_BLOCK_SIZE));
    }
    tst_assert(!buddy_alloc(size * BUDDY_BLOCK_SIZE));

    for (int i = 0; i < num_blocks / 32; i++)
    {
        buddy_free(ptr[i], size * BUDDY_BLOCK_SIZE);
    }
    buddy_print_memory_offsets();
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

TEST_SUITE_START(buddy)
{
    SUITE_ADD(full_range_memory);
    SUITE_ADD(out_of_mem);
    SUITE_ADD(alloc_free);
    SUITE_ADD(full_alloc_free);
    SUITE_ADD(full_alloc_free_32);
}
TEST_SUITE_END
