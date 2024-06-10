#include "vm_block.h"
#include "align.h"
#include "platform_malloc.h"
#include "element_build.h"
#include "timer.h"

static const TSize DEFAULT_PAGE_SIZE       = 4096;
static const TSize DEFAULT_ARENA_PAGE_SIZE = 65536;
static const TSize DEFAULT_ARENA_SIZE      = 256 * 1024 * 1024;

TPageMalloc* TPageMalloc::GPageMalloc = nullptr;

TPageMalloc* TPageMalloc::GetPageMalloc()
{
	static TPlatformMalloc PlatformMalloc;
	static TPageMalloc PageMalloc;

	if (!GPageMalloc)
	{
		if (PlatformMalloc.Init())
		{
			if (PageMalloc.Init(&PlatformMalloc))
			{
				GPageMalloc = &PageMalloc;
			}
		}
	}

	return GPageMalloc;
}

TPageMalloc::TPageMalloc() 
{
	ArenaPageSize = 0;
	PageSize      = 0;
	ArenaMinSize  = 0;
	PlatformMalloc = nullptr;
}

TPageMalloc::TArena::TArenaPageTable::TArenaPageTable() 
{
	PageCount = 0;
	FirstPage = nullptr;
	LastPage  = nullptr;
	PlatformMalloc = nullptr;
}

bool TPageMalloc::TArena::TArenaPageTable::Init(
	IPlatformMalloc* PlatformMalloc, TSize AreaSize, TSize ArenaPageSizeShift)
{
	TSize BlockCount = AreaSize >> ArenaPageSizeShift;
	TSize DataBlockSize = AlignToUpper(BlockTypeSize * BlockCount, PlatformMalloc->GetPageSize());

	bool Ok = PlatformMalloc->AllocateAndCommitMemoryBlock(DataBlockSize, DataBlock);
	if (Ok)
	{
		FirstPage = (TBlock*)DataBlock.GetBase();
		LastPage = FirstPage + (BlockCount - 1);

		this->PageCount = BlockCount;
		this->PlatformMalloc = PlatformMalloc;
		CreateElementsDefault(FirstPage, BlockCount);
		return true;
	}

	return false;
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::TArenaPageTable::GetArenaPage(TSize Index)
{
	if (Index < PageCount)
	{
		return FirstPage + Index;
	}

	return nullptr;
}

TSize TPageMalloc::TArena::TArenaPageTable::GetArenaPageIndex(TBlock* Node)
{
	return Node - FirstPage;
}

TSize TPageMalloc::TArena::TArenaPageTable::GetArenaPageCount()
{
	return PageCount;
}

void TPageMalloc::TArena::TArenaPageTable::Free()
{
	CreateElementsDefault(FirstPage, PageCount);
}

bool TPageMalloc::TArena::TArenaPageTable::Release()
{
	bool Ok = PlatformMalloc->DeallocateMemoryBlock(DataBlock);
	if (Ok)
	{
		FirstPage = nullptr;
		LastPage = nullptr;
		PageCount = 0;
		DataBlock = {};
	}

	return Ok;
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::TArenaPageTable::GetFirst()
{
	return FirstPage;
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::TArenaPageTable::GetLast()
{
	return LastPage;
}

bool TPageMalloc::TArena::Init(TSize AreaSize, TSize ArenaPageSize, TSize PageSize,
	IPlatformMalloc* PlatformMalloc, TPageMallocTimeStats* OutTimeStats)
{
	if (Initialized)
	{
		return false;
	}

	if (ArenaPageSize % PageSize)
	{
		return false;
	}

	/*if (AreaSize % ArenaPageSize)
		return false;*/

	TSize ArenaPageSizeShift = Log2_64(ArenaPageSize);

	bool Ok = ArenaPages.Init(PlatformMalloc, AreaSize, ArenaPageSizeShift);
	if (Ok)
	{
		Ok = PlatformMalloc->AllocateAndCommitMemoryBlock(AreaSize, Arena);

		void* AreaAddress = Arena.GetBase();
		TSize AllocatedAreaSize = Arena.GetSize();

		//for (TSize i = 0; i < AllocatedAreaSize; i += PageSize)
		//{
		//	*((uint8*)AreaAddress + i) = 0;
		//}

		if (Ok)
		{
			TBlock* FirstFreeBlock = ArenaPages.GetFirst();
			FreeBlockList.PushBack(FirstFreeBlock);


			//AreaAddress = Arena.GetBase();

			FirstFreeBlock->Ptr = AreaAddress;
			FirstFreeBlock->Size = AllocatedAreaSize;
			FirstFreeBlock->State = RELEASED;
			FirstFreeBlock->Upper = nullptr;
			FirstFreeBlock->Lower = nullptr;

			++FreeBlockCount;
			this->ArenaPageSize = ArenaPageSize;
			this->PlatformMalloc = PlatformMalloc;
			this->ArenaPageSizeShift = ArenaPageSizeShift;
			RestFreeSize = AreaSize;
			this->LastTimeStats = OutTimeStats;

			Initialized = true;

#if PAGE_MALLOC_DEBUG
			printf("PAGE MALLOC: DBG: Init memory area: At address: %p; Area size: %llu Area page size %llu - [ OK ]\n", AreaAddress, AreaSize, ArenaPageSize);
#endif
			return true;
		}
	}


#if PAGE_MALLOC_DEBUG
	printf("PAGE MALLOC: DBG: Init memory area: Area size: %llu Area page size %llu - [ FAILED ]\n", AreaSize, ArenaPageSize);
#endif
	return false;
}

TPageMalloc::TArena::TArena() :
	RestFreeSize(0),
	ArenaPageSize(0),
	ArenaPageSizeShift(0),
	UserBlockCount(0),
	FreeBlockCount(0),
	PlatformMalloc(nullptr),
	Initialized(false),
	LastTimeStats(nullptr)
{

}

bool TPageMalloc::TArena::IsInitialized()
{
	return Initialized;
}

void* TPageMalloc::TArena::TryMallocBlock(TSize Size, TSize& OutSize, void* Address)
{
#if PAGE_MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif
	if (!Size)
	{
		return nullptr;
	}

	if (Size > RestFreeSize)
	{
		return nullptr;
	}

	void* Ptr = nullptr;
	TBlock* FoundBlock = nullptr;
	TSize AlignedSize = AlignToUpper(Size, ArenaPageSize);

	if (Address)
	{
		void* AlignedAddress = AlignToLower(Address, ArenaPageSize);
		if (AlignedAddress != Address)
		{
			AlignedSize += ArenaPageSize;
		}

		FoundBlock = GetFreeBlock(AlignedAddress, AlignedSize);

		if (FoundBlock)
		{
			Ptr = AllocateBlock(FoundBlock, AlignedAddress, AlignedSize);
		}
	}
	else
	{
		FoundBlock = GetFreeBlock(AlignedSize);

		if (FoundBlock)
		{
			Ptr = AllocateBlock(FoundBlock, AlignedSize);
		}
	}

	if (Ptr)
	{
		OutSize = AlignedSize;
		RestFreeSize -= AlignedSize;
	}

#if PAGE_MALLOC_TIME_STATS
	Timer.Stop();
	LastTimeStats->AllocRequestTime += Timer.GetDuration();
	std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
	printf("PAGE MALLOC: DBG: Allocation block size: %llu Address: 0x%p;  Time : %f ms\n", Size, Address, d.count());

#endif

	return Ptr;
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::GetFreeBlock(TSize AlignedSize)
{
	auto BlockBase = FreeBlockList.GetFirst();
	TBlock* Block = nullptr;

	while (BlockBase)
	{
		Block = *BlockBase->GetElement();

		if (Block->Size >= AlignedSize)
		{
			return Block;
		}

		BlockBase = BlockBase->GetNext();
	}
	return nullptr;
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::GetFreeBlock(void* Address, TSize AlignedSize)
{
	//if (Address && ::IsPartOf(Address, Arena.GetBase(), Arena.GetSize()))
	{
		//if (IsAligned(Address, ArenaPageSize))
		{
			auto BlockBase = FreeBlockList.GetFirst();
			TBlock* Block = nullptr;

			while (BlockBase)
			{
				Block = *BlockBase->GetElement();
				uint8_t* LowerBorder = (uint8_t*)(Block->Ptr);
				uint8_t* UpperBorder = LowerBorder + Block->Size;

				if (IsPartOf(Address, AlignedSize, LowerBorder, UpperBorder))
				{
					return Block;
				}

				BlockBase = BlockBase->GetNext();
			}
		}
	}
	return nullptr;
}

void* TPageMalloc::TArena::AllocateBlock(TBlock* Block, TSize CommitedSize)
{
	void* ValidPtr = nullptr;

	TBlock* RestBlock = SplitReleasedBlock(Block, CommitedSize);

	if (RestBlock)
	{
		RestBlock->State = RELEASED;
	}
	Block->State = ALLOCATED;
	++UserBlockCount;
	--FreeBlockCount;
	FreeBlockList.Delete(Block);
	ValidPtr = Block->Ptr;

	return ValidPtr;
}

void* TPageMalloc::TArena::AllocateBlock(TBlock* Block, void* Address, TSize CommitedSize)
{
	void* ValidPtr = nullptr;

	// !!! correct Address to low border of AreaPage (64K)

		TSize FirstSplitedSize = (uint8_t*)Address - (uint8_t*)Block->Ptr;

		TBlock* UserBlock = nullptr;
		TBlock* RestBlock = nullptr;

		if (FirstSplitedSize)
		{
			UserBlock = SplitReleasedBlock(Block, FirstSplitedSize);
			Block->State = RELEASED;
			RestBlock = SplitReleasedBlock(UserBlock, CommitedSize);
		}
		else
		{
			RestBlock = SplitReleasedBlock(Block, CommitedSize);
			UserBlock = Block;
		}

		if (RestBlock)
		{
			RestBlock->State = RELEASED;
		}

		UserBlock->State = ALLOCATED;
		++UserBlockCount;
		--FreeBlockCount;
		FreeBlockList.Delete(UserBlock);
		ValidPtr = Address;
	
	return ValidPtr;
}

bool TPageMalloc::TArena::IsPartOf(void* Addr, TSize Size, void* LowerBorder, void* UpperBorder)
{
	uint8_t* Upper = (uint8_t*)Addr + Size;
	return (LowerBorder <= Addr && Upper <= UpperBorder);
}

TPageMalloc::TArena::TBlock* TPageMalloc::TArena::SplitReleasedBlock(TBlock* ParentBlock, TSize SizeToSplit)
{
	TBlock* RestBlock = nullptr;

	if (SizeToSplit)
	{
		TSize ParentBlockSize = ParentBlock->Size;

		if (ParentBlockSize > SizeToSplit)
		{
			TSize RIdx = SizeToSplit >> ArenaPageSizeShift;
			TSize BIdx = ArenaPages.GetArenaPageIndex(ParentBlock);

			RestBlock = ArenaPages.GetArenaPage(BIdx + RIdx);

			RestBlock->Ptr = (uint8_t*)(ParentBlock->Ptr) + SizeToSplit;
			RestBlock->Size = ParentBlockSize - SizeToSplit;

			TBlock* Upper = ParentBlock->Upper;
			RestBlock->Lower = ParentBlock;
			RestBlock->Upper = Upper;

			if (Upper)
			{
				Upper->Lower = RestBlock;
			}

			ParentBlock->Upper = RestBlock;

			ParentBlock->Size = SizeToSplit;

			FreeBlockList.PushBack(RestBlock);
			++FreeBlockCount;
		}
	}
	return RestBlock;
}

void TPageMalloc::TArena::MergeAdjecentReleasedBlocks(TBlock* Block)
{
	if (Block->Upper)
	{
		TBlock* UpperBlock = Block->Upper;
		if (UpperBlock->State == RELEASED)
		{
			FreeBlockList.Delete(UpperBlock);
			--FreeBlockCount;

			Block->Upper = UpperBlock->Upper;

			if (UpperBlock->Upper)
			{
				UpperBlock->Upper->Lower = Block;
			}

			UpperBlock->Upper = nullptr;
			UpperBlock->Lower = nullptr;

			Block->Size += UpperBlock->Size;
		}
	}

	if (Block->Lower)
	{
		TBlock* LowerBlock = Block->Lower;
		if (LowerBlock->State == RELEASED)
		{
			FreeBlockList.Delete(Block);
			--FreeBlockCount;

			LowerBlock->Upper = Block->Upper;

			if (Block->Upper)
			{
				Block->Upper->Lower = LowerBlock;
			}

			Block->Upper = nullptr;
			Block->Lower = nullptr;

			LowerBlock->Size += Block->Size;
		}
	}
}

bool TPageMalloc::TArena::TryFreeBlock(void* Address)
{
#if PAGE_MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif
	TSize BlkSize = 0;
	if (Address)
	{
		//	if (IsAligned(Address, ArenaPageSize))
		{
			if (::IsPartOf(Address, Arena.GetBase(), Arena.GetSize()))
			{
				void* AlignedAddress = AlignToLower(Address, ArenaPageSize);

				TSize BlockIdx = ((uint8_t*)AlignedAddress - (uint8_t*)Arena.GetBase()) >> ArenaPageSizeShift;

				TBlock* Block = ArenaPages.GetArenaPage(BlockIdx);
				BlkSize = Block->Size;
				RestFreeSize += Block->Size;
				Block->State = RELEASED;
				--UserBlockCount;
				++FreeBlockCount;
				FreeBlockList.PushBack(Block);
				MergeAdjecentReleasedBlocks(Block);

#if PAGE_MALLOC_TIME_STATS
				Timer.Stop();
				LastTimeStats->AllocRequestTime += Timer.GetDuration();
				std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
				printf("PAGE MALLOC: DBG: Releasing block size: %llu Address: 0x%p;  Time : %f ms\n", BlkSize, Address, d.count());

#endif
				return true;
				
			}
		}
	}
#if PAGE_MALLOC_TIME_STATS
	Timer.Stop();
	LastTimeStats->AllocRequestTime += Timer.GetDuration();
	std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
	printf("PAGE MALLOC: DBG: Releasing block size: %llu Address: 0x%p;  Time : %f ms\n", BlkSize, Address, d.count());
#endif
	return false;
}

TSize TPageMalloc::TArena::GetArenaSize()
{
	return Arena.GetSize();
}

void* TPageMalloc::TArena::GetArenaBase()
{
	return Arena.GetBase();
}

TSize TPageMalloc::TArena::GetArenaPageSize()
{
	return ArenaPageSize;
}

bool TPageMalloc::TArena::IsEmpty()
{
	return UserBlockCount == 0;
}

void TPageMalloc::TArena::Free()
{
	FreeBlockList.Delete();
	ArenaPages.Free();
	TBlock* ZeroBlock = ArenaPages.GetArenaPage(0);

	ZeroBlock->Size = Arena.GetSize();
	ZeroBlock->Ptr = Arena.GetBase();
	ZeroBlock->State = RELEASED;

	RestFreeSize = Arena.GetSize();
	UserBlockCount = 0;
	FreeBlockCount = 1;
	FreeBlockList.PushBack(ZeroBlock);
}

bool TPageMalloc::TArena::Release()
{
	bool Ok = PlatformMalloc->DeallocateMemoryBlock(TPlatformMemoryBlock(Arena.GetBase(), 0));
	if (Ok)
	{
		FreeBlockList.Delete();
		ArenaPages.Release();
		UserBlockCount = 0;
		FreeBlockCount = 0;
		RestFreeSize = 0;
		Arena = {};
		Initialized = false;
	}

	return Ok;
}

bool TPageMalloc::Init(IPlatformMalloc* PlatformMalloc, TSize ArenaPageSize, TSize PageSize, TSize ArenaMinSize)
{

#if PAGE_MALLOC_DEBUG
	printf("PAGE MALLOC: DBG: Initializing mem page allocator...\n");
#endif
	if (PlatformMalloc)
	{
		if (PlatformMalloc->IsPagingSupported())
		{
			this->PageSize = PlatformMalloc->GetPageSize();
		}
		else
		{
			if (PageSize)
			{
				this->PageSize = PageSize;
			}
			else
			{
				this->PageSize = DEFAULT_PAGE_SIZE;
			}
		}

		if (ArenaPageSize)
		{
			this->ArenaPageSize = ArenaPageSize;
		}
		else
		{
			this->ArenaPageSize = DEFAULT_ARENA_PAGE_SIZE;
		}

#if PAGE_MALLOC_STATS
		Stats.ArenaPageSize = this->ArenaPageSize;
#endif

		if (ArenaMinSize)
		{
			this->ArenaMinSize = ArenaMinSize;
		}
		else
		{
			this->ArenaMinSize = DEFAULT_ARENA_SIZE;
		}

		this->PlatformMalloc = PlatformMalloc;
#if PAGE_MALLOC_STATS
		Stats.MaxArenaCount = PAGE_MALLOC_MAX_ARENA_COUNT;
#endif

		bool Ok = ArenaTable.Init(PlatformMalloc, PAGE_MALLOC_MAX_ARENA_COUNT);
		if (!Ok)
		{
			return false;
		}
		
		
		return true;
	}
	return false;
}


TSize TPageMalloc::GetMaxArenaCount()
{
	return PAGE_MALLOC_MAX_ARENA_COUNT;
}

bool TPageMalloc::Reserve(TMemoryBlock& OutBlock)
{
	return Reserve(ArenaMinSize, OutBlock);
}

bool TPageMalloc::Reserve(TSize Size, TMemoryBlock& OutBlock)
{
#if PAGE_MALLOC_STATS
	++Stats.ReserveRequests;
#endif

	bool Ok = false;
	int32_t FreeSlot = ArenaTable.GetFreeSlot();

	if (FreeSlot != INVALID_SLOT)
	{
		TSize AlignedSize = AlignToUpper(Size, ArenaPageSize);
		if (AlignedSize > ArenaMinSize)
		{
			Ok = ArenaTable[FreeSlot]->Arena.Init(AlignedSize, ArenaPageSize, PageSize, PlatformMalloc, &LastTimeStats);
		}
		else
		{
			Ok = ArenaTable[FreeSlot]->Arena.Init(ArenaMinSize, ArenaPageSize, PageSize, PlatformMalloc, &LastTimeStats);
		}

		if (Ok)
		{
			ArenaTable[FreeSlot]->Free = false;
			OutBlock = TMemoryBlock{ ArenaTable[FreeSlot]->Arena.GetArenaBase(), ArenaTable[FreeSlot]->Arena.GetArenaSize() };

#if PAGE_MALLOC_DEBUG
			printf("PAGE MALLOC: DBG: Reserve memory block: Size: %llu - [ OK ]\n", Size);
#endif

#if PAGE_MALLOC_STATS
			++Stats.ArenaCount;

			Stats.TotalReservedSize += ArenaTable[FreeSlot]->Arena.GetArenaSize();
			//	if (AlignedSize > Stats.MaxBlockSizeToReserve)
			//		Stats.MaxBlockSizeToReserve = AlignedSize;
			if (ArenaTable[FreeSlot]->Arena.GetArenaSize() > Stats.ArenaMaxSize)
			{
				Stats.ArenaMaxSize = ArenaTable[FreeSlot]->Arena.GetArenaSize();
			}

			if (ArenaTable[FreeSlot]->Arena.GetArenaSize() < Stats.ArenaMinSize)
			{
				Stats.ArenaMinSize = ArenaTable[FreeSlot]->Arena.GetArenaSize();
			}

#endif
		}
	}

#if PAGE_MALLOC_DEBUG
	if (!Ok)
	{
		printf("PAGE MALLOC: DBG: Reserve memory block: Size: %llu - [ FAILED ]\n", Size);
	}

#endif
#if PAGE_MALLOC_STATS
	if (!Ok)
	{
		++Stats.FailedReserveRequests;
	}

#endif
	return Ok;
}

bool TPageMalloc::AllocateBlock(TSize Size, TMemoryBlock& OutBlock, void* AreaBaseAddr)
{
#if PAGE_MALLOC_STATS
	++Stats.AllocRequests;
#endif

	TSize AllocatedSize;
	void* Ptr = nullptr;
	bool Ok = false;
	Ptr = TryAllocateBlock(Size, AllocatedSize, AreaBaseAddr);

	if (Ptr)
	{
		OutBlock = TMemoryBlock(Ptr, AllocatedSize);
		Ok = true;
#if PAGE_MALLOC_STATS
		Stats.TotalUsedSize += AllocatedSize;
		if (Size > Stats.MaxBlockSizeToAlloc)
		{
			Stats.MaxBlockSizeToAlloc = Size;
		}

#endif
#if PAGE_MALLOC_DEBUG
		printf("PAGE MALLOC: DBG: Alloc memory block: size: %llu area base addr: 0x%p - [ OK ]\n", Size, AreaBaseAddr);
#endif
	}
	else
	{
#if PAGE_MALLOC_DEBUG
		printf("PAGE MALLOC: DBG: Alloc memory block: size: %llu area base addr: 0x%p - [ FAILED ]\n", Size, AreaBaseAddr);
#endif
#if PAGE_MALLOC_STATS
		++Stats.FailedAllocRequests;
#endif
	}
	return Ok;
}

bool TPageMalloc::AllocateBlock(void* Address, TSize Size, TMemoryBlock& OutBlock)
{
#if PAGE_MALLOC_STATS
	++Stats.AllocRequests;
#endif
	TSize AllocatedSize;
	void* Ptr = nullptr;
	bool Ok = false;
	Ptr = TryAllocateBlock(Address, Size, AllocatedSize);

	if (Ptr)
	{
		OutBlock = TMemoryBlock(Address, AllocatedSize);
		Ok = true;

#if PAGE_MALLOC_STATS
		Stats.TotalUsedSize += AllocatedSize;
		if (Size > Stats.MaxBlockSizeToAlloc)
		{
			Stats.MaxBlockSizeToAlloc = Size;
		}

#endif
#if PAGE_MALLOC_DEBUG
		printf("PAGE MALLOC: DBG: Alloc memory block: size: %llu at address: 0x%p - [ OK ]\n", Size, Address);
#endif
	}
	else
	{
#if PAGE_MALLOC_STATS
		++Stats.FailedAllocRequests;
#endif
#if PAGE_MALLOC_DEBUG
		printf("PAGE MALLOC: DBG: Alloc memory block: size: %llu at address: 0x%p - [ FAILED ]\n", Size, Address);
#endif
	}

	return Ok;
}

void* TPageMalloc::TryAllocateBlock(TSize Size, TSize& OutSize, void* AreaBaseAddr)
{
	void* Ptr = nullptr;
	if (AreaBaseAddr)
	{
		for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
		{
			if (!ArenaTable[i]->Free)
			{
				if (ArenaTable[i]->Arena.GetArenaBase() == AreaBaseAddr)
				{
					Ptr = ArenaTable[i]->Arena.TryMallocBlock(Size, OutSize, nullptr);
					break;
				}
			}
		}
	}
	else
	{
		for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
		{
			if (!ArenaTable[i]->Free)
			{
				Ptr = ArenaTable[i]->Arena.TryMallocBlock(Size, OutSize, nullptr);

				if (Ptr)
				{
					break;
				}

			}
		}
	}

	return Ptr;
}

void* TPageMalloc::TryAllocateBlock(void* Address, TSize Size, TSize& OutSize)
{

	void* Ptr = nullptr;
	for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
	{
		if (!ArenaTable[i]->Free)
		{
			if (::IsPartOf(Address, ArenaTable[i]->Arena.GetArenaBase(), ArenaTable[i]->Arena.GetArenaSize()))
			{
				Ptr = ArenaTable[i]->Arena.TryMallocBlock(Size, OutSize, Address);
				break;
			}
		}
	}
	return Ptr;
}

bool TPageMalloc::FreeBlock(TMemoryBlock Block)
{
#if PAGE_MALLOC_STATS
	++Stats.FreeRequests;
#endif

	bool Ok = false;

	for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
	{
		if (!ArenaTable[i]->Free)
		{
			Ok = ArenaTable[i]->Arena.TryFreeBlock(Block.GetBase());
			if (Ok)
			{
				if (ArenaTable[i]->Arena.IsEmpty())
				{
					void* ArenaAddr = ArenaTable[i]->Arena.GetArenaBase();
					Ok = ArenaTable[i]->Arena.Release();

					if (Ok)
					{
						ArenaTable[i]->Free = true;
					}
					else
					{
						printf("PAGE MALLOC: WARNING: CANNOT RELEASE VM ARENA BACK TO OPERATING SYSTEM: %p; POSSIBLE LACK OF MEMORY\n", ArenaAddr);
					}
				}

				//Ok = true;
#if PAGE_MALLOC_STATS
				Stats.TotalUsedSize -= Block.GetSize();
				if (Block.GetSize() > Stats.MaxBlockSizeToFree)
					Stats.MaxBlockSizeToFree = Block.GetSize();
#endif
#if PAGE_MALLOC_DEBUG

				printf("PAGE MALLOC: DBG: Free memory block: size: %llu - [ OK ]\n", Block.GetSize());
#endif
				break;
			}
		}
	}

#if PAGE_MALLOC_DEBUG
	if (!Ok)
	{
		printf("PAGE MALLOC: DBG: Free memory block: size: %llu - [ FAILED ]\n", Block.GetSize());
	}

#endif
#if PAGE_MALLOC_STATS
	if (!Ok)
	{
		++Stats.FailedFreeRequests;
	}
#endif

	return Ok;
}

bool TPageMalloc::SetProtection(TMemoryBlock Block, TMemoryBlockAccess Protect)
{
	return PlatformMalloc->SetMemBlockProtection(TPlatformMemoryBlock(Block.GetBase(), Block.GetSize()), Protect);
}

bool TPageMalloc::IsProtectionSupported()
{
	return PlatformMalloc->IsProtectionSupported();
}

TSize TPageMalloc::GetGranularity()
{
	return ArenaPageSize;
}

TSize TPageMalloc::GetPageSize()
{
	return PageSize;
}

bool TPageMalloc::Free()
{
	bool Ok = false;
	for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
	{
		if (!ArenaTable[i]->Free)
		{
			ArenaTable[i]->Arena.Free();
		}
	}

#if PAGE_MALLOC_DEBUG
	if (Ok)
	{
		printf("PAGE MALLOC: DBG: Free page allocator - [ OK ]\n");
	}
	else
	{
		printf("PAGE MALLOC: DBG: Free page allocator - [ FAILED ]\n");
	}

#endif

#if PAGE_MALLOC_STATS
	if (Ok)
	{
		Stats = TPageMallocStats{};
	}
#endif

	return Ok;
}

bool TPageMalloc::Release(void* AreaBaseAddr)
{
	// not implemented yet;
	return true;
}

bool TPageMalloc::Release()
{

	bool Ok = false;
	for (TSize i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
	{
		if (!ArenaTable[i]->Free)
		{
			Ok = ArenaTable[i]->Arena.Release();
			if (Ok)
			{
				ArenaTable[i]->Free = true;
			}
			else
			{
				//log warning;
				break;
			}

		}
	}

#if PAGE_MALLOC_DEBUG
	if (Ok)
	{
		printf("PAGE MALLOC: DBG: Release page allocator - [ OK ]\n");
	}
	else
	{
		printf("PAGE MALLOC: DBG: Release page allocator - [ FAILED ]\n");
	}

#endif

#if PAGE_MALLOC_STATS
	if (Ok)
	{
		Stats = TPageMallocStats{};
	}

#endif

	return Ok;
}


void TPageMalloc::GetStats(TPageMallocStats& OutStats)
{
#if PAGE_MALLOC_STATS
	Guard.Lock();
	OutStats = Stats;
	Guard.Unlock();
#else
	printf("PAGE MALLOC: INF: Page allocator stats are not enabled\n");
#endif
}

bool TPageMalloc::TArenaTable::Init(IPlatformMalloc* PlatformMalloc, TSize ArenaCount)
{
	TSize DataBlockSize = AlignToUpper(ArenaSlotSize * ArenaCount, PlatformMalloc->GetPageSize());

	bool Ok = PlatformMalloc->AllocateAndCommitMemoryBlock(DataBlockSize, DataBlock);

	if (Ok)
	{
		FirstArena = (TArenaSlot*)DataBlock.GetBase();
		LastArena = FirstArena + (ArenaCount - 1);

		this->ArenaCount = ArenaCount;
		this->PlatformMalloc = PlatformMalloc;
		CreateElementsDefault(FirstArena, ArenaCount);
		return true;
	}

	return false;
}


TSize TPageMalloc::TArenaTable::GetArenaSlotIndex(TArenaSlot* Node)
{
	return Node - FirstArena;
}

TSize TPageMalloc::TArenaTable::GetArenaSlotCount()
{
	return ArenaCount;
}

int32 TPageMalloc::TArenaTable::GetFreeSlot()
{
	for (int32 i = 0; i < ArenaCount; ++i)
	{
		if (FirstArena[i].Free)
		{
			return i;
		}
	}

	return INVALID_SLOT;
}

void TPageMalloc::TArenaTable::Free()
{
	for (int32 i = 0; i < ArenaCount; ++i)
	{
		FirstArena[i].Free = true;
	}
}

bool TPageMalloc::TArenaTable::Release()
{
	for (int32 i = 0; i < ArenaCount; ++i)
	{
		bool Ok = FirstArena[i].Arena.Release();
		if (!Ok)
		{
			return false;
		}

		FirstArena[i].Free = true;
	}

	return true;
}

void TPageMalloc::GetLastTimeStats(TPageMallocTimeStats& OutLastTimeStats)
{
#if PAGE_MALLOC_TIME_STATS
	Guard.Lock();
	OutLastTimeStats = LastTimeStats;
	Guard.Unlock();
#else
	printf("PAGE MALLOC: INF: Page allocator time stats are not enabled\n");
#endif
}

#if	PAGE_MALLOC_DEBUG
TSize TPageMalloc::GetArenaDefaultSize()
{
	return DEFAULT_ARENA_SIZE;
}

void TPageMalloc::DumpMemMap(void* AreaBaseAddr)
{
	for (uint32_t i = 0; i < PAGE_MALLOC_MAX_ARENA_COUNT; ++i)
	{
		if (ArenaTable[i]->Arena.GetArenaBase() == AreaBaseAddr)
		{
			if (!ArenaTable[i]->Free)
			{
				ArenaTable[i]->Arena.DumpMemMap();
			}
			break;
		}
	}
}

void TPageMalloc::TArena::DumpMemMap()
{
	TBlock* bfirst = ArenaPages.GetFirst();
	uint32_t BcInPerRow = 8;

	const char* Bstate[2] = { "ALLOCATED","RELEASED" };

	printf("PAGE MALLOC: DBG: Memory area map. Area %p\n", Arena.GetBase());

	for (auto block = bfirst; block != nullptr; block = block->Upper)
	{
		printf("| -------------------\n");
		printf("|\t0x%p\t\n", block->Ptr);
		printf("|\t%llu\t\n", block->Size);
		uint32_t State = block->State;
		printf("|\t%s\t\n", Bstate[State]);

	}
}
#endif