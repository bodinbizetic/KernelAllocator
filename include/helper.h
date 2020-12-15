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

#define FLS(num) (num ? sizeof(num) * CHAR_BIT - __builtin_clz(num) : 0)

// Find first >= POW_TWO
#define BEST_FIT_BLOCKID(num) (num == 0 ? 0 : FLS(num - 1))

#define POWER_OF_TWO(num) ((num & (num - 1)) == 0)
#define ROUND_TO_POWER_OF_TWO(num) (1 << (BEST_FIT_BLOCKID(num)))

//******************************************************************//
// ASSERTION *******************************************************//
#include <assert.h>

#define ASSERT(exp) assert(exp)

//******************************************************************//
// CUSTOM TYPES *******************************************************//

typedef char bool;
#define true 1
#define false 0

#endif // __HELPER_H
