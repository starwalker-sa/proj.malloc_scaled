#pragma once

#include "build.h"

#if PLATFORM_WIN
#include "Win\win_platform_critical_section.h"
#endif

#if PLATFORM_UNIX
#include "Unix\unix_platform_critical_section.h"
#endif

