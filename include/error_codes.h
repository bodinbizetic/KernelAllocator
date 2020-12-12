#ifndef __ERROR_CODES_H
#define __ERROR_CODES_H

enum Result_Code
{
    OK = 0,
    PARAM_ERROR = 1,
    NOT_ENOUGH_MEMORY = 2,
    NOT_ENOUGH_MEMORY_TO_INIT = 4,
    SYSTEM_NOT_INITIALIZED = 8,
    SYSTEM_ALREADY_INITIALIZED = 16
};

void printCode(int code);

#endif // __ERROR_CODES_H