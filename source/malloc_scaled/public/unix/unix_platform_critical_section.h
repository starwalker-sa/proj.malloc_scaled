#pragma once

#include "build.h"
#include <mutex>

#if PLATFORM_UNIX
class TUnixPlatformCriticalSection
{
public:
	TUnixPlatformCriticalSection() = default;
	~TUnixPlatformCriticalSection() = default;

	void LockSection();
	bool TryLockSection();
	void UnlockSection();
private:
	std::mutex CriticalSection;
};

using TPlatformCriticalSection = TUnixPlatformCriticalSection;
#endif
