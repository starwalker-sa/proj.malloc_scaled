// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/MallocScaledVMBlock.h"
//#include "HAL/PlatformMemory.h"

bool TMallocScaledVMBlock::Init()
{
	return true;
}

bool TMallocScaledVMBlock::Allocate(SIZE_T Size)
{
	if (!Allocated)
	{
		BlockBase = FPlatformMemory::BinnedAllocFromOS(Size);

		if (BlockBase)
		{
			Allocated = true;
			return true;
		}
	}

	return false;
}

void TMallocScaledVMBlock::Deallocate()
{
	FPlatformMemory::BinnedFreeToOS(BlockBase, BlockSize);
	Allocated = false;
}

bool TMallocScaledVMBlock::Commit()
{
	return true;
}

bool TMallocScaledVMBlock::Decommit()
{
	return true; 
}

bool TMallocScaledVMBlock::Commit(void* Offset, SIZE_T Size)
{
	return true;
}

bool TMallocScaledVMBlock::Decommit(void* Offset, SIZE_T Size)
{
	return true;
}

bool TMallocScaledVMBlock::SetProtection(void* Offset, SIZE_T Size, TMemoryBlockAccess AccessFlag)
{
	bool bCanRead = false;
	bool bCanWrite = false;

	switch (AccessFlag)
	{
	case TMallocScaledVMBlock::TMemoryBlockAccess::NoAccess:
		break;
	case TMallocScaledVMBlock::TMemoryBlockAccess::ReadOnly:
		bCanRead = true;
		break;
	case TMallocScaledVMBlock::TMemoryBlockAccess::ReadWrite:
		bCanRead = true;
		bCanWrite = true;
		break;
	case TMallocScaledVMBlock::TMemoryBlockAccess::Execute:
		break;
	case TMallocScaledVMBlock::TMemoryBlockAccess::FullAccess:
		break;
	default:
		break;
	}

	return FPlatformMemory::PageProtect(Offset, Size, bCanRead, bCanWrite);
}

bool TMallocScaledVMBlock::IsPagingSupported()
{
	return true;
}

bool TMallocScaledVMBlock::IsProtectionSupported()
{
	return true;
}

SIZE_T TMallocScaledVMBlock::GetPageSize()
{
	return FPlatformMemory::GetConstants().PageSize;
}

bool TMallocScaledVMBlock::IsAllocated()
{
	return Allocated;
}

SIZE_T TMallocScaledVMBlock::GetAllocatedSize()
{
	return BlockSize;
}

void* TMallocScaledVMBlock::GetBase()
{
	return BlockBase;
}

void* TMallocScaledVMBlock::GetEnd()
{
	return (uint8*)BlockBase + BlockSize;
}