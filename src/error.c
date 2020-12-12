#include "error_codes.h"

void printCode(int code)
{
    if (code & OK)
        printf("OK\n");
    else if (code & PARAM_ERROR)
        printf("PARAM_ERROR\n");
    else if (code & NOT_ENOUGH_MEMORY)
        printf("NOT_ENOUGH_MEMORY\n");
    else if (code & NOT_ENOUGH_MEMORY_TO_INIT)
        printf("NOT_ENOUGH_MEMORY_TO_INIT\n");
    else if (code & SYSTEM_NOT_INITIALIZED)
        printf("SYSTEM_NOT_INITIALIZED\n");
    else if (code & SYSTEM_ALREADY_INITIALIZED)
        printf("SYSTEM_ALREADY_INITIALIZED\n");
}