#pragma once

#include "std.h"

class TMemoryBlock
{
public:
	TMemoryBlock() :
		Size(0),
		Base(nullptr)
	{

	}


	TMemoryBlock(void* Ptr, TSize Size):
		Size(Size),
		Base(Ptr)
	{

	}

	void* GetBase() const
	{
		return Base;
	}

	TSize GetSize() const
	{
		return Size;
	}

	void* GetEnd() const
	{
		return (uint8_t*)Base + Size;
	}

private:
	TSize Size;
	void* Base;
};

