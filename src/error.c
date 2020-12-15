#include "error_codes.h"
#include <stdio.h>

void printCode(int code)
{
    if (code & OK)
        printf("OK\n");
    if (code & PARAM_ERROR)
        printf("PARAM_ERROR\n");
    if (code & NOT_ENOUGH_MEMORY)
        printf("NOT_ENOUGH_MEMORY\n");
    if (code & NOT_ENOUGH_MEMORY_TO_INIT)
        printf("NOT_ENOUGH_MEMORY_TO_INIT\n");
    if (code & SYSTEM_NOT_INITIALIZED)
        printf("SYSTEM_NOT_INITIALIZED\n");
    if (code & SYSTEM_ALREADY_INITIALIZED)
        printf("SYSTEM_ALREADY_INITIALIZED\n");
    if (code & SLAB_OBJECT_BIGGER_THAN_BLOCK)
        printf("SLAB_OBJECT_BIGGER_THAN_BLOCK\n");
    if (code & SLAB_FULL)
        printf("SLAB_FULL\n");
    if (code & SLAB_DEALLOC_NOT_VALID_ADDRES)
        printf("SLAB_DEALLOC_NOT_VALID_ADDRES = 128\n");
}