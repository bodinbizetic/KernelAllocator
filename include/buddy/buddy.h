#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "error_codes.h"
#include "helper.h"
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#define BUDDY_BLOCK_SIZE 4096

typedef struct buddy_block_struct
{
    struct buddy_block_struct *prev;
    struct buddy_block_struct *next;
    uint8_t blockid;
} buddy_block_t;

typedef struct buddy_table_entry_struct
{
    buddy_block_t *block;
    uint8_t *bitmap;
    size_t numByteMap;
} buddy_table_entry_t;

typedef struct buddy_allocator_struct
{
    void *vpStart;
    size_t totalSize;
    uint8_t maxBlockSize;
    void *vpMemoryStart;
    size_t memorySize;
    buddy_table_entry_t *vpMemoryBlocks;
    CRITICAL_SECTION CriticalSection;
} buddy_allocator_t;

CRESULT buddy_init(void *vpSpace, size_t size);
CRESULT buddy_destroy();
CRESULT buddy_alloc(size_t size, void **result);
CRESULT buddy_free(void *ptr, size_t size);

void buddy_print_memory_offsets();
void buddy_print_bitmap();
#if defined(LOGING) && defined(LOGING_BUDDY)

#define BUDDY_LOG(str, ...) LOG(str, __VA_ARGS__)

#else
#define BUDDY_LOG(str, ...)
#endif // BUDDY_LOG

#define PRINT()                                                                                                        \
    buddy_print_bitmap();                                                                                              \
    buddy_print_memory_offsets();

#endif
