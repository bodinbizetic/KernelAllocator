#include "buddy/buddy.h"

int main()
{
    void *ptr = malloc(127 + 24);
    buddy_init(ptr, 127 + 24);
    return 0;
}