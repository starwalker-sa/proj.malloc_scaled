#pragma once

#include "platform_malloc.h"
#include "build.h"

#if PLATFORM_UNIX
class TUnixPlatformMalloc :
	public IPlatformMalloc
{
public:
	TUnixPlatformMalloc() = default;

	virtual bool Init();

	virtual bool AllocateMemoryBlock(TSize Size, TPlatformMemoryBlock& OutBlock);
	virtual bool AllocateMemoryBlock(void* Address, TSize Size, TPlatformMemoryBlock& OutBlock);

	virtual bool DeallocateMemoryBlock(TPlatformMemoryBlock InBlock);

	virtual bool CommitMemoryBlock(TPlatformMemoryBlock InBlock);
	virtual bool DecommitMemoryBlock(TPlatformMemoryBlock InBlock);

	virtual bool SetMemBlockProtection(TPlatformMemoryBlock InBlock, TMemoryBlockAccess ProtFlag);

	virtual bool IsPagingSupported();
	virtual bool IsProtectionSupported();

	virtual TSize GetPageSize();
private:
	static int32 TranslatePageProtection(TMemoryBlockAccess Access);
};
using TPlatformMalloc = TUnixPlatformMalloc;
#endif
