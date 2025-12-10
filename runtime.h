#ifndef FLUKE_H
#define FLUKE_H

#include "clam.h"

extern const void *const process_base;
extern const void *const process_limit;

#define unlikely(x) __builtin_expect(!!(x), 0)

#define BOUNDS_FN_ATTR                                                         \
  __attribute__((always_inline)) __attribute__((visibility("hidden")))
BOUNDS_FN_ATTR const void *__bounds_check(const void *ptr, long size);
BOUNDS_FN_ATTR void __bounds_assume(const void *ptr, long size);

#endif
