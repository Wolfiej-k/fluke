#ifndef FLUKE_H
#define FLUKE_H

#include "clam.h"
#include <stdlib.h>

extern const void *process_base;
extern const void *process_limit;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define ENTRY_FN_ATTR __attribute__((visibility("default")))
#define BOUNDS_FN_ATTR __attribute__((always_inline))

ENTRY_FN_ATTR void entry(void);
BOUNDS_FN_ATTR void __bounds_check(const void *ptr, long size);
BOUNDS_FN_ATTR void __bounds_assume(const void *ptr, long size);

#endif
