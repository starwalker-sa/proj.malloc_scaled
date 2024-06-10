// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "PlatformMemory.h"

class TMallocScaledVMBlock
{
public:

	enum class TMemoryBlockAccess
	{
		NoAccess,
		ReadOnly,
		ReadWrite,
		Execute,
		FullAccess
	};


	TMallocScaledVMBlock():
		Allocated(false),
		BlockSize(0),
		BlockBase(nullptr)
	{

	}
	TMallocScaledVMBlock(TMallocScaledVMBlock&&) = default;
	TMallocScaledVMBlock& operator=(TMallocScaledVMBlock&&) = default;

	TMallocScaledVMBlock(TMallocScaledVMBlock&) = delete;
	TMallocScaledVMBlock& operator=(TMallocScaledVMBlock&) = delete;



	bool Allocate(SIZE_T Size);

	void Deallocate(); // !!! Destroy whole block;

	bool Commit();
	bool Decommit();

	bool Commit(void* Offset, SIZE_T Size);
	bool Decommit(void* Offset, SIZE_T Size);

	bool SetProtection(void* Offset, SIZE_T Size, TMemoryBlockAccess AccessFlag);

	bool IsAllocated();

	static bool Init();
	static bool IsPagingSupported();
	static bool IsProtectionSupported();

	static SIZE_T GetPageSize();

	void* GetBase();
	void* GetEnd();

	SIZE_T GetAllocatedSize();

private:
	bool Allocated;
	SIZE_T BlockSize;
	void*  BlockBase;
};