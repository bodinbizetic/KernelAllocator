#ifndef __HELPER_H
#define __HELPER_H

#include <stdio.h>

#define LOG(str, ...) printf(str, __VA_ARGS__); \
                        printf("\n");

#endif // __HELPER_H