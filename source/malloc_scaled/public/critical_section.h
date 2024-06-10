#pragma once

#include "platform_critical_section.h"

class TCriticalSection :
	private TPlatformCriticalSection
{
public:

	TCriticalSection() = default;
	~TCriticalSection() = default;

	void Lock()
	{
		LockSection();
	}

	bool TryLock()
	{
		TryLockSection();
	}

	void Unlock()
	{
		UnlockSection();
	}
};
