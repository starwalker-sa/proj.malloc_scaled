#include "win_platform_critical_section.h"

#ifdef PLATFORM_WIN
TWinPlatformCriticalSection::TWinPlatformCriticalSection()
{
	InitializeCriticalSection(&CriticalSection);
	SetCriticalSectionSpinCount(&CriticalSection, SpinCount);
}

void TWinPlatformCriticalSection::LockSection()
{
	EnterCriticalSection(&CriticalSection);
}

bool TWinPlatformCriticalSection::TryLockSection()
{
	return TryEnterCriticalSection(&CriticalSection);
}

void TWinPlatformCriticalSection::UnlockSection()
{
	LeaveCriticalSection(&CriticalSection);
}

TWinPlatformCriticalSection::~TWinPlatformCriticalSection()
{
	DeleteCriticalSection(&CriticalSection);
}
#endif