#include "unix_platform_critical_section.h"

#if PLATFORM_UNIX

void TUnixPlatformCriticalSection::LockSection()
{
	CriticalSection.lock();
}

bool TUnixPlatformCriticalSection::TryLockSection()
{
	return CriticalSection.try_lock();
}

void TUnixPlatformCriticalSection::UnlockSection()
{
	CriticalSection.unlock();
}

#endif