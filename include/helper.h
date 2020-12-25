#ifndef __HELPER_H
#define __HELPER_H

#include <stdio.h>

//******************************************************************//
// LOGING *******************************************************//

#define LOGING
//#define LOGING_BUDDY
#define LOGING_SLAB

#ifdef LOGING

#define LOG(str, ...)                                                                                                  \
    printf(str, __VA_ARGS__);                                                                                          \
    printf("\n")
#else

#define LOG(str, ...)

#endif // LOGING

//******************************************************************//
// BIT HELPER *******************************************************//
#if defined(WIN32) || defined(WIN64)
#include <stdint.h>

static uint64_t __inline clz(uint64_t value)
{
    int sol = 0;
    while (value)
    {
        sol++;
        value >>= 1;
    }
    return sol;
}

#define FLS(num) clz(num)
#else
#define FLS(num) (num ? sizeof(int) * CHAR_BIT - __builtin_clz(num) : 0)
#endif

// Find first >= POW_TWO
#define BEST_FIT_BLOCKID(num) (num == 0 ? 0 : FLS(num - 1))

#define POWER_OF_TWO(num) ((num & (num - 1)) == 0)
#define ROUND_TO_POWER_OF_TWO(num) (1 << (BEST_FIT_BLOCKID(num)))

//******************************************************************//
// ASSERTION *******************************************************//
#include <assert.h>

#define ASSERT(exp) assert(exp)
//#define ASSERT(exp)

//******************************************************************//
// CUSTOM TYPES *******************************************************//

typedef char bool;
#define true 1
#define false 0

#endif // __HELPER_H
