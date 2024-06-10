
#include "align.h"
#include "win\win_platform_malloc.h"

#if PLATFORM_WIN

DWORD TWinPlatformMalloc::TranslatePageProtection(TMemoryBlockAccess Access)
{
	DWORD Pr = 0;
	switch (Access)
	{
	case TMemoryBlockAccess::NoAccess:
		Pr = PAGE_NOACCESS;
		break;
	case TMemoryBlockAccess::ReadOnly:
		Pr = PAGE_READONLY;
		break;
	case TMemoryBlockAccess::ReadWrite:
		Pr = PAGE_READWRITE;
		break;
	case TMemoryBlockAccess::Execute:
		Pr = PAGE_EXECUTE;
		break;
	case TMemoryBlockAccess::FullAccess:
		Pr = PAGE_EXECUTE_READWRITE;
		break;
	default:
		break;
	}
	return Pr;
}

bool TWinPlatformMalloc::Init()
{
	SYSTEM_INFO SysInfo = {};
	GetSystemInfo(&SysInfo);

	PageSize = SysInfo.dwPageSize;

	if (PageSize)
	{
		bIsPagingSupported = true;
	}

	bIsProtectionSupported = true;

	return true;
}

bool TWinPlatformMalloc::AllocateMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignToUpper(Size, PageSize);
	Ptr = VirtualAlloc(NULL, AlignedBlockSize, MEM_RESERVE, PAGE_READWRITE);

	if (Ptr != NULL)
	{
		OutBlock = TPlatformMemoryBlock{ Ptr, AlignedBlockSize };
		return true;
	}

	return false;
}

bool TWinPlatformMalloc::AllocateMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignToUpper(Size, PageSize);
	Ptr = VirtualAlloc(Address, AlignedBlockSize, MEM_RESERVE, PAGE_READWRITE);
	
	if (Ptr != NULL)
	{
		OutBlock = TPlatformMemoryBlock(Ptr, AlignedBlockSize);
		return true;
	}

	return false;
}


bool TWinPlatformMalloc::CommitMemoryBlock(TPlatformMemoryBlock InBlock)
{
	if (!InBlock.GetBase())
	{
		return false;
	}

	void* Ptr = VirtualAlloc(InBlock.GetBase(), InBlock.GetSize(), MEM_COMMIT, PAGE_READWRITE);

	if (Ptr != NULL)
	{
		return true;
	}

	return false;
}

bool TWinPlatformMalloc::AllocateAndCommitMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignToUpper(Size, PageSize);
	Ptr = VirtualAlloc(NULL, AlignedBlockSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (Ptr != NULL)
	{
		OutBlock = TPlatformMemoryBlock(Ptr, AlignedBlockSize);
		return true;
	}

	return false;
}

bool TWinPlatformMalloc::AllocateAndCommitMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock)
{
	if (!Size)
	{
		return false;
	}

	void* Ptr = nullptr;
	TSize AlignedBlockSize = AlignToUpper(Size, PageSize);
	Ptr = VirtualAlloc(Address, AlignedBlockSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (Ptr != NULL)
	{
		OutBlock = TPlatformMemoryBlock(Ptr, AlignedBlockSize);
		return true;
	}

	return false;
}

#pragma warning(disable:6250)
bool TWinPlatformMalloc::DecommitMemoryBlock(TPlatformMemoryBlock InBlock)
{
	if (!InBlock.GetBase())
	{
		return false;
	}

	BOOL Ok = VirtualFree(InBlock.GetBase(), InBlock.GetSize(), MEM_DECOMMIT);

	return (bool)Ok;
}
#pragma warning(default:6250)

bool TWinPlatformMalloc::SetMemBlockProtection(TPlatformMemoryBlock InBlock, TMemoryBlockAccess ProtFlag)
{
	if (!InBlock.GetBase())
	{
		return false;
	}

	DWORD Prot = TranslatePageProtection(ProtFlag);
	DWORD OldProt;
	bool Protected = VirtualProtect(InBlock.GetBase(), InBlock.GetSize(), Prot, &OldProt);

	if (Protected)
	{
		return true;
	}

	return false;
}

bool TWinPlatformMalloc::IsPagingSupported()
{
	return bIsPagingSupported;
}

bool TWinPlatformMalloc::IsProtectionSupported()
{
	return bIsProtectionSupported;
}

bool TWinPlatformMalloc::DeallocateMemoryBlock(TPlatformMemoryBlock InBlock)
{
	void* Base = InBlock.GetBase();
	if (!Base)
	{
		return false;
	}

	BOOL Ok = VirtualFree(Base, 0, MEM_RELEASE);

	return (bool)Ok;
}

TSize TWinPlatformMalloc::GetPageSize()
{
	return PageSize;
}

#endif