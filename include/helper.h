#ifndef __HELPER_H
#define __HELPER_H

#include <stdio.h>

#define LOGING
#define LOGING_BUDDY


#ifdef LOGING

#define LOG(str, ...) printf(str, __VA_ARGS__); \
                        printf("\n")
#else

#define LOG(str, ...)

#endif // LOGING


#if defined(LOGING) && defined(LOGING_BUDDY)

#define BUDDY_LOG(str, ...) LOG(str, __VA_ARGS__)

#else
#define BUDDY_LOG(str, ...)
#endif // BUDDY_LOG


#define FLS(num) sizeof(num) * CHAR_BIT - __builtin_clz(num)

#define BEST_FIT_BLOCKID(num) FLS(num-1) + 1

#define POWER_OF_TWO(num) ((num & (num - 1)) == 0)
#define ROUND_TO_POWER_OF_TWO(num) (1  << (BEST_FIT_BLOCKID(num)-1))
#endif // __HELPER_H
