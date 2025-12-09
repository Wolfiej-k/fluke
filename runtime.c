#include "runtime.h"

BOUNDS_FN_ATTR void __bounds_check(const void *ptr, long size) {
  int base_ok = (long)process_base <= (long)ptr;
  int limit_ok = (long)process_limit >= (long)ptr + size;

  __CRAB_assert(base_ok);
  __CRAB_assert(limit_ok); 

  int ok = base_ok & limit_ok;
  if (unlikely(!ok)) {
    __builtin_trap();
  }
}

BOUNDS_FN_ATTR void __bounds_assume(const void *ptr, long size) {
  __CRAB_assume((long)process_base <= (long)ptr);
  __CRAB_assume((long)process_limit >= (long)ptr + size);
}
