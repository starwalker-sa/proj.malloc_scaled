
#include "align.h"
#include "unix\unix_platform_malloc.h"

#if PLATFORM_UNIX

#include <sys/mman.h>
#include <unistd.h>

int32 TUnixPlatformMalloc::TranslatePageProtection(TMemoryBlockAccess Access)
{
	int32 Pr = 0;
	switch (Access)
	{
	case TMemoryBlockAccess::NoAccess:
		Pr = PROT_NONE;
		break;
	case TMemoryBlockAccess::ReadOnly:
		Pr = PROT_READ;
		break;
	case TMemoryBlockAccess::ReadWrite:
		Pr = PROT_WRITE | PROT_READ;
		break;
	case TMemoryBlockAccess::Execute:
		Pr = PROT_EXEC;
		break;
	case TMemoryBlockAccess::FullAccess:
		Pr = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
	default:
		break;
	}
	return Pr;
}

bool TUnixPlatformMalloc::Init()
{
	PageSize = getpagesize();

	if (PageSize)
	{
		bIsPagingSupported = true;
	}

	bIsProtectionSupported = true;

	return true;
}

bool TUnixPlatformMalloc::AllocateMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignSizeToUpper(Size, PageSize);
	Ptr = mmap(nullptr, AlignedBlockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (Ptr != MAP_FAILED)
	{
		OutBlock = TPlatformMemoryBlock(Ptr, AlignedBlockSize);
		return true;
	}

	return false;
}

bool TUnixPlatformMalloc::AllocateMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignSizeToUpper(Size, PageSize);
	Ptr = mmap(Address, AlignedBlockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (Ptr != MAP_FAILED)
	{
		OutBlock = TPlatformMemoryBlock(Ptr, AlignedBlockSize);
		return true;
	}

	return false;
}


bool TUnixPlatformMalloc::CommitMemoryBlock(TPlatformMemoryBlock InBlock)
{
	return true;
}

bool TUnixPlatformMalloc::DecommitMemoryBlock(TPlatformMemoryBlock InBlock)
{
	return true;
}

bool TUnixPlatformMalloc::SetMemBlockProtection(TPlatformMemoryBlock InBlock, TMemoryBlockAccess ProtFlag)
{
	if (!InBlock.GetBase())
	{
		return false;
	}

	int32 Prot = TranslatePageProtection(ProtFlag);
	bool Protected = mprotect(InBlock.GetBase(), InBlock.GetSize(), Prot);

	if (Protected)
	{
		return true;
	}
	
	return false;
}

bool TUnixPlatformMalloc::IsPagingSupported()
{
	return bIsPagingSupported;
}

bool TUnixPlatformMalloc::IsProtectionSupported()
{
	return bIsProtectionSupported;
}

bool TUnixPlatformMalloc::DeallocateMemoryBlock(TPlatformMemoryBlock InBlock)
{
	if (!InBlock.GetBase())
	{
		return false;
	}

	bool Ok = munmap(InBlock.GetBase(), InBlock.GetSize());

	return Ok;
}

TSize TUnixPlatformMalloc::GetPageSize()
{
	return PageSize;
}

#endif