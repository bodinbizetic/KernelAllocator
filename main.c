#include "buddy/buddy.h"
#include "helper.h"

int main()
{
    const int MEMORY_SIZE = (32) * BLOCK_SIZE + 88;
    void *ptr1 = malloc(MEMORY_SIZE);
    buddy_init(ptr1, MEMORY_SIZE);
    return 0;
}
