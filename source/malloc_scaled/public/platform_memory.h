#pragma once

#include "build.h"

#if PLATFORM_WIN
#include "win\win_platform_malloc.h"
#elif PLATFORM_UNIX
#include "unix\unix_platform_malloc.h"
#endif


