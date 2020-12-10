#include "buddy/buddy.h"
#include "helper.h"

int main()
{
    const int MEMORY_SIZE = (33) * BUDDY_BLOCK_SIZE;
    void *ptr1 = malloc(MEMORY_SIZE);
    buddy_init(ptr1, MEMORY_SIZE);
    return 0;
}
