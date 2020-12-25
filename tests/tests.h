#ifndef __TEST_H
#define __TEST_H

#include <stdio.h>

#define TEST_MEM_CAP 1000

#define BUDDY_TEST_START(name)                                                                                         \
    static bool tst_##name(size_t Max_Blocks)                                                                          \
    {                                                                                                                  \
        printf("\n-> Started test %s\n", #name);                                                                       \
        const size_t MEMORY_SIZE = (Max_Blocks)*BUDDY_BLOCK_SIZE;                                                      \
        void *_ptr = malloc(MEMORY_SIZE);                                                                              \
        buddy_init(_ptr, MEMORY_SIZE);                                                                                 \
        size_t total_blocks_mem = MEMORY_SIZE - (size_t)s_pBuddyHead->vpMemoryStart + (size_t)s_pBuddyHead->vpStart;   \
        size_t num_blocks = total_blocks_mem / BUDDY_BLOCK_SIZE;

#define BUDDY_TEST_END                                                                                                 \
    buddy_destroy();                                                                                                   \
    free(_ptr);                                                                                                        \
    printf("Done\n");                                                                                                  \
    return true;                                                                                                       \
    }

#define NUMBER_OF_OBJECTS_IN_SLAB_VIRTUAL(objSize, toStore)                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        kmem_slab_t *slab;                                                                                             \
        size_t _l1Offset = 0;                                                                                          \
        get_slab(objSize, &_l1Offset, &slab);                                                                          \
        toStore = NUMBER_OF_OBJECTS_IN_SLAB(slab);                                                                     \
    } while (0)

#define SLAB_TEST_START(name)                                                                                          \
    static bool tst_##name(size_t Max_Blocks, size_t Obj_Size)                                                         \
    {                                                                                                                  \
        printf("\n-> Started test %s\n", #name);                                                                       \
        const size_t MEMORY_SIZE = (Max_Blocks);                                                                       \
        void *_ptr = malloc(MEMORY_SIZE * BLOCK_SIZE);                                                                 \
        const size_t objSize = Obj_Size;                                                                               \
        kmem_init(_ptr, MEMORY_SIZE);                                                                                  \
        int _numberOfObjectsInSlab;                                                                                    \
        NUMBER_OF_OBJECTS_IN_SLAB_VIRTUAL(objSize, _numberOfObjectsInSlab);

#define SLAB_TEST_END                                                                                                  \
    buddy_destroy();                                                                                                   \
    free(_ptr);                                                                                                        \
    printf("Done\n");                                                                                                  \
    return true;                                                                                                       \
    }

#define TEST_SUITE_START(name, MemSize)                                                                                \
    bool suite_##name()                                                                                                \
    {                                                                                                                  \
        const int Mem_Size = MemSize;                                                                                  \
        short cnt = 0;                                                                                                 \
        printf("/************Started %s suite************/\n", #name);                                                 \
        int tst_num_cnt = 0;

#define TEST_SUITE_END                                                                                                 \
    printf("\n\nSucceeded: %d\nFailed: %d\n", cnt, tst_num_cnt - cnt);                                                 \
    printf("/************Ended suite************/\n");                                                                 \
    return cnt == tst_num_cnt;                                                                                         \
    }

#define SUITE_ADD(name, ...)                                                                                           \
    cnt += tst_##name(Mem_Size);                                                                                       \
    tst_num_cnt++;

#define SUITE_ADD_OBJSIZE(name, Size)                                                                                  \
    cnt += tst_##name(Mem_Size, Size);                                                                                 \
    tst_num_cnt++;

#define tst_assert(x)                                                                                                  \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        printf("Assertion failed\nFile: %s:%d\nFunction: %s\n\n", __FILE__, __LINE__, __FUNCTION__);                   \
        return false;                                                                                                  \
    }

#define tst_OK(x) tst_assert(!(x))
#define tst_FAIL(x) tst_assert(x)

#endif // __TEST_H
