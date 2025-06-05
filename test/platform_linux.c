#include "assert.h"

#define _POSIX_C_SOURCE 199309L
#include <time.h>

internalfn u64
NowInNanoseconds(void)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts))
    runtime_assert(0 && "clock is unstable");

  return (u64)ts.tv_sec * 1000000000 /* 1e9 */ + (u64)ts.tv_nsec;
}
