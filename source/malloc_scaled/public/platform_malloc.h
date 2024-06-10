#pragma once

#include "std.h"

enum class TMemoryBlockAccess
{
	NoAccess,
	ReadOnly,
	ReadWrite,
	Execute,
	FullAccess
};

class TPlatformMemoryBlock
{
public:
	TPlatformMemoryBlock():
		Base(nullptr),
		Size(0)
	{

	}

	TPlatformMemoryBlock(void* Ptr, TSize Size):
		Base(Ptr),
		Size(Size)
	{

	}

	void* GetBase() const
	{
		return Base;
	}

	void* GetEnd() const
	{
		return (uint8_t*)Base + Size;
	}

	TSize GetSize() const
	{
		return Size;
	}

private:
	TSize Size;
	void* Base;
};


class IPlatformMalloc
{
public:

	IPlatformMalloc() :
		PageSize(0),
		bIsPagingSupported(false),
		bIsProtectionSupported(false)
	{

	}

	virtual bool Init() = 0;

	virtual bool AllocateMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock) = 0;
	virtual bool AllocateMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock) = 0;

	virtual bool AllocateAndCommitMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock) = 0;
	virtual bool AllocateAndCommitMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock) = 0;

	virtual bool DeallocateMemoryBlock(TPlatformMemoryBlock InBlock) = 0;

	virtual bool CommitMemoryBlock(TPlatformMemoryBlock InBlock) = 0;
	virtual bool DecommitMemoryBlock(TPlatformMemoryBlock InBlock) = 0;

	virtual bool SetMemBlockProtection(TPlatformMemoryBlock Block, TMemoryBlockAccess AccessFlag) = 0;

	virtual bool IsPagingSupported() = 0;
	virtual bool IsProtectionSupported() = 0;

	virtual TSize GetPageSize() = 0;

	virtual ~IPlatformMalloc() = default;

protected:
	TSize PageSize;
	bool bIsPagingSupported;
	bool bIsProtectionSupported;
};

