#pragma once

#include "std.h"

class IMalloc
{
public:

	virtual void* Malloc(TSize Size, TSize Alignment) = 0;
	virtual void* Realloc(void* Addr, TSize NewSize, TSize NewAlignment) = 0;
	virtual void  Free(void* Addr) = 0;
	virtual TSize GetSize(void* Addr) = 0;

};
