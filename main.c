#include "buddy/buddy.h"
#include "helper.h"

int main()
{
    const int MEMORY_SIZE = (127 + 24 + 36) * BLOCK_SIZE;
    void *ptr = malloc(MEMORY_SIZE);
    buddy_init(ptr, MEMORY_SIZE);
    //printf("%d %d", sizeof(1) * CHAR_BIT, FLS(1));
    return 0;
}