#ifndef __CUTILS_BITOPS_H
#define __CUTILS_BITOPS_H

#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/cdefs.h>


static inline int popcount(unsigned int x) {
    return __builtin_popcount(x);
}

static inline int popcountl(unsigned long x) {
    return __builtin_popcountl(x);
}

static inline int popcountll(unsigned long long x) {
    return __builtin_popcountll(x);
}


#endif
