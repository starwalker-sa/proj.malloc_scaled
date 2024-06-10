#pragma once

#include "build.h"
#include "win.h"

#if PLATFORM_WIN
class TWinPlatformCriticalSection
{
public:
	TWinPlatformCriticalSection();
	~TWinPlatformCriticalSection();

	void LockSection();
	bool TryLockSection();
	void UnlockSection();
private:
	CRITICAL_SECTION CriticalSection;
	static const DWORD SpinCount = 4000;
};

using TPlatformCriticalSection = TWinPlatformCriticalSection;
#endif