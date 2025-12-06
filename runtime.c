#include "runtime.h"

BOUNDS_FN_ATTR void __bounds_check(const void *ptr, long size) {
  if (unlikely(size == 0)) {
    return;
  }

  int above_base = (char *)process_base <= (char *)ptr;
  int below_limit = (char *)process_limit >= (char *)ptr + size;
  __CRAB_assert(above_base);
  __CRAB_assert(below_limit);

  if (unlikely(!above_base || !below_limit)) {
    abort();
  }
}

BOUNDS_FN_ATTR void __bounds_assume(const void *ptr, long size) {
  __CRAB_assume((char *)process_base <= (char *)ptr);
  __CRAB_assume((char *)process_limit >= (char *)ptr + size);
}
