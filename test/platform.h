#include "type.h"

internalfn u64
NowInNanoseconds(void);

#if IS_PLATFORM_LINUX
#include "platform_linux.c"
#elif IS_PLATFORM_WINDOWS
#include "platform_windows.c"
#endif
