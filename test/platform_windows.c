#include "assert.h"

#include <profileapi.h>

internalfn u64
NowInNanoseconds(void)
{
  static LARGE_INTEGER frequency = {0};

  if (frequency.QuadPart == 0)
    if (!QueryPerformanceFrequency(&frequency))
      runtime_assert(0 && "clock is unstable");

  LARGE_INTEGER tick;
  if (!QueryPerformanceCounter(&tick))
    runtime_assert(0 && "clock is unstable");

  return (u64)(tick.QuadPart * (1000000000 /*1e9*/ / frequency.QuadPart));
}
