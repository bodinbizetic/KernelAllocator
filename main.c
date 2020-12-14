#include "buddy/buddy.h"
#include "helper.h"
#include "slab.h"
#include "slab_impl.h"
#include <windows.h>

int main()
{
    const int BLOCK_SIZE_INIT = 33;
    const int MEMORY_SIZE = (33) * BUDDY_BLOCK_SIZE;
    void *ptr1 = malloc(MEMORY_SIZE);
    kmem_init(ptr1, 33);
    kmalloc(32);
    return 0;
}
