#ifndef _BUDDY_H_
#define _BUDDY_H_

#include <stdlib.h>

#define BLOCK_SIZE 4096

void buddy_init(void* vpSpace, size_t size);


#endif
