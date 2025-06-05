#include "type.h"

internalfn u64
NowInNanoseconds(void);

#if IS_PLATFORM_LINUX
#include "platform_linux.c"
#endif
