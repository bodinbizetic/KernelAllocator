#ifndef _BUDDY_H_
#define _BUDDY_H_

#include <stdlib.h>
#include "helper.h"

#define BLOCK_SIZE 256

void buddy_init(void *vpSpace, size_t size);
void *buddy_alloc(size_t size);

void buddy_print_memory_offsets();


#if defined(LOGING) && defined(LOGING_BUDDY)

#define BUDDY_LOG(str, ...) LOG(str, __VA_ARGS__)

#else
#define BUDDY_LOG(str, ...)
#endif // BUDDY_LOG

#endif
