#include "buddy/buddy.h"
#include "helper.h"

int main()
{
    const int MEMORY_SIZE = (32) * BUDDY_BLOCK_SIZE;
    void *ptr1 = malloc(MEMORY_SIZE);
    buddy_init(ptr1, MEMORY_SIZE);
    buddy_print_bitmap();
    buddy_print_memory_offsets();
    void *ptr2 = buddy_alloc(4 * BUDDY_BLOCK_SIZE);
    void *ptr3 = buddy_alloc(2 * BUDDY_BLOCK_SIZE);
    buddy_free(ptr2, 4 * BUDDY_BLOCK_SIZE);
    buddy_free(ptr3, 2 * BUDDY_BLOCK_SIZE);
    return 0;
}
