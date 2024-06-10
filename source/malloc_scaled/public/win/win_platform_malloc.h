#pragma once

#include "platform_malloc.h"
#include "build.h"
#include <win.h>

#if PLATFORM_WIN

class TWinPlatformMalloc :
	public IPlatformMalloc
{
public:
	TWinPlatformMalloc() = default;

	virtual bool Init();

	virtual bool AllocateMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock);
	virtual bool AllocateMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock);

	virtual bool AllocateAndCommitMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock);
	virtual bool AllocateAndCommitMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock);

	virtual bool DeallocateMemoryBlock(TPlatformMemoryBlock InBlock);

	virtual bool CommitMemoryBlock(TPlatformMemoryBlock InBlock);
	virtual bool DecommitMemoryBlock(TPlatformMemoryBlock InBlock);

	virtual bool SetMemBlockProtection(TPlatformMemoryBlock InBlock, TMemoryBlockAccess ProtFlag);

	virtual bool IsPagingSupported();
	virtual bool IsProtectionSupported();

	virtual TSize GetPageSize();
private:
	static DWORD TranslatePageProtection(TMemoryBlockAccess Access);
};
using TPlatformMalloc = TWinPlatformMalloc;
#endif
