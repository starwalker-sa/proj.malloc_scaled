#pragma once

#include "malloc_scaled.h"
#include "defs.h"

enum EMAllocToUse
{
	None,
	MallocScaled1,
	MallocScaled2
};

class TMemoryAllocator
{
public:
	static IMalloc* GetMalloc();


	static TMallocScaled* GetMallocObject();


	static bool Init(EMAllocToUse MallocToUse);
	static void Shutdown();
	static TMemoryAllocator* GetMemoryAllocator();

private:
	TMemoryAllocator()
	{

	}

	static TMemoryAllocator* GMemoryAllocator;
	static TMallocBase* GMalloc;
	static EMAllocToUse MallocToUse;
};

