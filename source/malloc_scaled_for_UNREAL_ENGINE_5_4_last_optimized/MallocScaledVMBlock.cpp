// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/MallocScaledVMBlock.h"
#include "HAL/PlatformMemory.h"
#include "Templates/AlignmentTemplates.h"
#include "Templates/MemoryOps.h"
//UE_DISABLE_OPTIMIZATION
IPageMalloc* FMallocScaledVMBlock::PageMalloc = nullptr;

bool FMallocScaledVMBlock::Init()
{
	if (!PageMalloc)
	{
		PageMalloc = FMallocScaledPageMalloc::GetPageMalloc();
		if (!PageMalloc)
		{
			return false;
		}
	}

	return true;
}

void FMallocScaledVMBlock::Release()
{
	if (PageMalloc)
	{
		PageMalloc->Release();
	}
}

bool FMallocScaledVMBlock::Allocate(SIZE_T Size)
{
	bool Ok = false;

	if (PageMalloc)
	{
		Ok = PageMalloc->AllocateBlock(Size, VMBlock);

		if (!Ok)
		{
			FMemoryBlock Block;
			Ok = PageMalloc->Reserve(Size, Block);

			if (Ok)
			{
				Ok = PageMalloc->AllocateBlock(Size, VMBlock);
			}
		}

		if (Ok)
		{
			Allocated = true;
			return true;
		}
	}

	return false;
}

bool FMallocScaledVMBlock::Allocate(void* Address, SIZE_T Size)
{
	return false;
}

void FMallocScaledVMBlock::Free()
{
	if (PageMalloc)
	{
		void* Base = VMBlock.GetBase();
		SIZE_T Size = VMBlock.GetSize();
		bool Ok = PageMalloc->FreeBlock(FMemoryBlock{ Base, Size });

		if (!Ok)
		{
			// log: POSSIBLE LACK OF MEMORY;
			//printf("PAGE MALLOC: CANNOT RELEASE BLOCK: Address: %p, Size: %llu; MIGHT BE LACK OF MEMORY\n",
			//	Base, Size);
		}
	}

	Allocated = false;
}

bool FMallocScaledVMBlock::SetProtection(void* Offset, SIZE_T Size, FVMBlockAccess AccessFlag)
{
	if (PageMalloc)
	{
		return PageMalloc->SetProtection(FMemoryBlock{ Offset, Size }, AccessFlag);
	}

	return false;
}

bool FMallocScaledVMBlock::IsPagingSupported()
{
	return (bool)PageMalloc->GetPageSize();
}

bool FMallocScaledVMBlock::IsProtectionSupported()
{
	return PageMalloc->IsProtectionSupported();
}

SIZE_T FMallocScaledVMBlock::GetPageSize()
{
	return PageMalloc->GetPageSize();
}

bool FMallocScaledVMBlock::IsAllocated()
{
	return Allocated;
}

SIZE_T FMallocScaledVMBlock::GetAllocatedSize()
{
	return VMBlock.GetSize();
}

void* FMallocScaledVMBlock::GetBase()
{
	return VMBlock.GetBase();
}

void* FMallocScaledVMBlock::GetEnd()
{
	return VMBlock.GetEnd();
}

static const SIZE_T MALLOC_SCALED_DEFAULT_PAGE_SIZE = 4096;
static const SIZE_T MALLOC_SCALED_DEFAULT_ARENA_PAGE_SIZE = 65536;
static const SIZE_T MALLOC_SCALED_DEFAULT_ARENA_SIZE = 256 * 1024 * 1024;

FMallocScaledPageMalloc* FMallocScaledPageMalloc::GPageMalloc = nullptr;

FMallocScaledPageMalloc* FMallocScaledPageMalloc::GetPageMalloc()
{
	static FMallocScaledPageMalloc PageMalloc;

	if (!GPageMalloc)
	{
		if (PageMalloc.Init())
		{
			GPageMalloc = &PageMalloc;
		}

	}

	return GPageMalloc;
}

FMallocScaledPageMalloc::FMallocScaledPageMalloc()
{
	ArenaPageSize = 0;
	PageSize = 0;
	ArenaMinSize = 0;
}

FMallocScaledPageMalloc::FArena::FArenaPageTable::FArenaPageTable()
{
	PageCount = 0;
	FirstPage = nullptr;
	LastPage = nullptr;
}

bool FMallocScaledPageMalloc::FArena::FArenaPageTable::Init(
	SIZE_T AreaSize, SIZE_T ArenaPageSizeShift)
{
	SIZE_T BlockCount = AreaSize >> ArenaPageSizeShift;
	SIZE_T DataBlockSize = Align(BlockTypeSize * BlockCount, 4096);

	void* BlkPtr = FPlatformMemory::BinnedAllocFromOS(DataBlockSize);
	if (BlkPtr)
	{
		DataBlock = { BlkPtr, DataBlockSize };
		FirstPage = (FBlock*)DataBlock.GetBase();
		LastPage = FirstPage + (BlockCount - 1);

		this->PageCount = BlockCount;
		FMallocScaledUtils::CreateElementsDefault(FirstPage, BlockCount);
		return true;
	}

	return false;
}

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::FArenaPageTable::GetArenaPage(SIZE_T Index)
{
	if (Index < PageCount)
	{
		return FirstPage + Index;
	}

	return nullptr;
}

SIZE_T FMallocScaledPageMalloc::FArena::FArenaPageTable::GetArenaPageIndex(FBlock* Node)
{
	return Node - FirstPage;
}

SIZE_T FMallocScaledPageMalloc::FArena::FArenaPageTable::GetArenaPageCount()
{
	return PageCount;
}

void FMallocScaledPageMalloc::FArena::FArenaPageTable::Free()
{
	DefaultConstructItems<FBlock>(FirstPage, PageCount);
}

bool FMallocScaledPageMalloc::FArena::FArenaPageTable::Release()
{
	FPlatformMemory::BinnedFreeToOS(DataBlock.GetBase(), DataBlock.GetSize());

	FirstPage = nullptr;
	LastPage = nullptr;
	PageCount = 0;
	DataBlock = {};

	return true;
}

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::FArenaPageTable::GetFirst()
{
	return FirstPage;
}

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::FArenaPageTable::GetLast()
{
	return LastPage;
}

bool FMallocScaledPageMalloc::FArena::Init(SIZE_T InArenaSize, SIZE_T InArenaPageSize, SIZE_T InPageSize,
	FPageMallocTimeStats* OutTimeStats)
{
	if (Initialized)
	{
		return false;
	}

	if (InArenaPageSize % InPageSize)
	{
		return false;
	}

	/*if (AreaSize % ArenaPageSize)
		return false;*/

	SIZE_T InArenaPageSizeShift = FMallocScaledMath::Log2_64(InArenaPageSize);

	bool Ok = ArenaPages.Init(InArenaSize, InArenaPageSizeShift);
	if (Ok)
	{
		void* BlkPtr = FPlatformMemory::BinnedAllocFromOS(InArenaSize);

		if (BlkPtr)
		{
			Arena = { BlkPtr, InArenaSize };

			void* AreaAddress = Arena.GetBase();
			SIZE_T AllocatedAreaSize = Arena.GetSize();

			FBlock* FirstFreeBlock = ArenaPages.GetFirst();
			FreeBlockList.PushBack(FirstFreeBlock);

			FirstFreeBlock->Ptr = AreaAddress;
			FirstFreeBlock->Size = AllocatedAreaSize;
			FirstFreeBlock->State = RELEASED;
			FirstFreeBlock->Upper = nullptr;
			FirstFreeBlock->Lower = nullptr;

			++FreeBlockCount;
			this->ArenaPageSize = InArenaPageSize;
			this->ArenaPageSizeShift = InArenaPageSizeShift;
			RestFreeSize = InArenaSize;
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

FMallocScaledPageMalloc::FArena::FArena() :
	RestFreeSize(0),
	FreeBlockCount(0),
	UserBlockCount(0),
	Initialized(false),
	ArenaPageSize(0),
	ArenaPageSizeShift(0),
	LastTimeStats(nullptr)
{

}

bool FMallocScaledPageMalloc::FArena::IsInitialized()
{
	return Initialized;
}

void* FMallocScaledPageMalloc::FArena::TryMallocBlock(SIZE_T Size, SIZE_T& OutSize, void* Address)
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
	FBlock* FoundBlock = nullptr;
	SIZE_T AlignedSize = Align(Size, ArenaPageSize);

	if (Address)
	{
		void* AlignedAddress = AlignDown(Address, ArenaPageSize);
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

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::GetFreeBlock(SIZE_T AlignedSize)
{
	auto BlockBase = FreeBlockList.GetFirst();
	FBlock* Block = nullptr;

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

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::GetFreeBlock(void* Address, SIZE_T AlignedSize)
{
	//if (Address && ::IsPartOf(Address, Arena.GetBase(), Arena.GetSize()))
	{
		//if (IsAligned(Address, ArenaPageSize))
		{
			auto BlockBase = FreeBlockList.GetFirst();
			FBlock* Block = nullptr;

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

void* FMallocScaledPageMalloc::FArena::AllocateBlock(FBlock* Block, SIZE_T CommitedSize)
{
	void* ValidPtr = nullptr;

	FBlock* RestBlock = SplitReleasedBlock(Block, CommitedSize);

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

void* FMallocScaledPageMalloc::FArena::AllocateBlock(FBlock* Block, void* Address, SIZE_T CommitedSize)
{
	void* ValidPtr = nullptr;

	// !!! correct Address to low border of AreaPage (64K)

	SIZE_T FirstSplitedSize = (uint8_t*)Address - (uint8_t*)Block->Ptr;

	FBlock* UserBlock = nullptr;
	FBlock* RestBlock = nullptr;

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

bool FMallocScaledPageMalloc::FArena::IsPartOf(void* Addr, SIZE_T Size, void* LowerBorder, void* UpperBorder)
{
	uint8_t* Upper = (uint8_t*)Addr + Size;
	return (LowerBorder <= Addr && Upper <= UpperBorder);
}

FMallocScaledPageMalloc::FArena::FBlock* FMallocScaledPageMalloc::FArena::SplitReleasedBlock(FBlock* ParentBlock, SIZE_T SizeToSplit)
{
	FBlock* RestBlock = nullptr;

	if (SizeToSplit)
	{
		SIZE_T ParentBlockSize = ParentBlock->Size;

		if (ParentBlockSize > SizeToSplit)
		{
			SIZE_T RIdx = SizeToSplit >> ArenaPageSizeShift;
			SIZE_T BIdx = ArenaPages.GetArenaPageIndex(ParentBlock);

			RestBlock = ArenaPages.GetArenaPage(BIdx + RIdx);

			RestBlock->Ptr = (uint8_t*)(ParentBlock->Ptr) + SizeToSplit;
			RestBlock->Size = ParentBlockSize - SizeToSplit;

			FBlock* Upper = ParentBlock->Upper;
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

void FMallocScaledPageMalloc::FArena::MergeAdjecentReleasedBlocks(FBlock* Block)
{
	if (Block->Upper)
	{
		FBlock* UpperBlock = Block->Upper;
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
		FBlock* LowerBlock = Block->Lower;
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

bool FMallocScaledPageMalloc::FArena::TryFreeBlock(void* Address)
{
#if PAGE_MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif
	SIZE_T BlkSize = 0;
	if (Address)
	{
		//	if (IsAligned(Address, ArenaPageSize))
		{
			if (::IsPartOf(Address, Arena.GetBase(), Arena.GetSize()))
			{
				void* AlignedAddress = AlignDown(Address, ArenaPageSize);

				SIZE_T BlockIdx = ((uint8_t*)AlignedAddress - (uint8_t*)Arena.GetBase()) >> ArenaPageSizeShift;

				FBlock* Block = ArenaPages.GetArenaPage(BlockIdx);
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

SIZE_T FMallocScaledPageMalloc::FArena::GetArenaSize()
{
	return Arena.GetSize();
}

void* FMallocScaledPageMalloc::FArena::GetArenaBase()
{
	return Arena.GetBase();
}

SIZE_T FMallocScaledPageMalloc::FArena::GetArenaPageSize()
{
	return ArenaPageSize;
}

bool FMallocScaledPageMalloc::FArena::IsEmpty()
{
	return UserBlockCount == 0;
}

void FMallocScaledPageMalloc::FArena::Free()
{
	FreeBlockList.Delete();
	ArenaPages.Free();
	FBlock* ZeroBlock = ArenaPages.GetArenaPage(0);

	ZeroBlock->Size = Arena.GetSize();
	ZeroBlock->Ptr = Arena.GetBase();
	ZeroBlock->State = RELEASED;

	RestFreeSize = Arena.GetSize();
	UserBlockCount = 0;
	FreeBlockCount = 1;
	FreeBlockList.PushBack(ZeroBlock);
}

bool FMallocScaledPageMalloc::FArena::Release()
{
	FPlatformMemory::BinnedFreeToOS(Arena.GetBase(), Arena.GetSize());

	FreeBlockList.Delete();
	ArenaPages.Release();
	UserBlockCount = 0;
	FreeBlockCount = 0;
	RestFreeSize = 0;
	Arena = {};
	Initialized = false;


	return true;
}

bool FMallocScaledPageMalloc::Init(SIZE_T InArenaPageSize, SIZE_T InPageSize, SIZE_T InArenaMinSize)
{

#if PAGE_MALLOC_DEBUG
	printf("PAGE MALLOC: DBG: Initializing mem page allocator...\n");
#endif
	if (true)
	{
		this->PageSize = 4096;
	}
	else
	{
		if (PageSize)
		{
			this->PageSize = InPageSize;
		}
		else
		{
			this->PageSize = MALLOC_SCALED_DEFAULT_PAGE_SIZE;
		}
	}

	if (InArenaPageSize)
	{
		this->ArenaPageSize = InArenaPageSize;
	}
	else
	{
		this->ArenaPageSize = MALLOC_SCALED_DEFAULT_ARENA_PAGE_SIZE;
	}

#if PAGE_MALLOC_STATS
	Stats.ArenaPageSize = this->ArenaPageSize;
#endif

	if (InArenaMinSize)
	{
		this->ArenaMinSize = InArenaMinSize;
	}
	else
	{
		this->ArenaMinSize = MALLOC_SCALED_DEFAULT_ARENA_SIZE;
	}


#if PAGE_MALLOC_STATS
	Stats.MaxArenaCount = PAGE_MALLOC_MAX_ARENA_COUNT;
#endif

	bool Ok = ArenaTable.Init(MALLOC_SCALED_MAX_ARENA_COUNT);
	if (!Ok)
	{
		return false;
	}

	return true;
}


SIZE_T FMallocScaledPageMalloc::GetMaxArenaCount()
{
	return MALLOC_SCALED_MAX_ARENA_COUNT;
}

bool FMallocScaledPageMalloc::Reserve(FMemoryBlock& OutBlock)
{
	return Reserve(ArenaMinSize, OutBlock);
}

bool FMallocScaledPageMalloc::Reserve(SIZE_T Size, FMemoryBlock& OutBlock)
{
#if PAGE_MALLOC_STATS
	++Stats.ReserveRequests;
#endif

	bool Ok = false;
	int32_t FreeSlot = ArenaTable.GetFreeSlot();

	if (FreeSlot != INVALID_SLOT)
	{
		SIZE_T AlignedSize = Align(Size, ArenaPageSize);
		if (AlignedSize > ArenaMinSize)
		{
			Ok = ArenaTable[FreeSlot]->Arena.Init(AlignedSize, ArenaPageSize, PageSize, &LastTimeStats);
		}
		else
		{
			Ok = ArenaTable[FreeSlot]->Arena.Init(ArenaMinSize, ArenaPageSize, PageSize, &LastTimeStats);
		}

		if (Ok)
		{
			ArenaTable[FreeSlot]->Free = false;
			OutBlock = FMemoryBlock{ ArenaTable[FreeSlot]->Arena.GetArenaBase(), ArenaTable[FreeSlot]->Arena.GetArenaSize() };

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

bool FMallocScaledPageMalloc::AllocateBlock(SIZE_T Size, FMemoryBlock& OutBlock, void* AreaBaseAddr)
{
#if PAGE_MALLOC_STATS
	++Stats.AllocRequests;
#endif

	SIZE_T AllocatedSize;
	void* Ptr = nullptr;
	bool Ok = false;
	Ptr = TryAllocateBlock(Size, AllocatedSize, AreaBaseAddr);

	if (Ptr)
	{
		OutBlock = FMemoryBlock(Ptr, AllocatedSize);
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

bool FMallocScaledPageMalloc::AllocateBlock(void* Address, SIZE_T Size, FMemoryBlock& OutBlock)
{
#if PAGE_MALLOC_STATS
	++Stats.AllocRequests;
#endif
	SIZE_T AllocatedSize;
	void* Ptr = nullptr;
	bool Ok = false;
	Ptr = TryAllocateBlock(Address, Size, AllocatedSize);

	if (Ptr)
	{
		OutBlock = FMemoryBlock(Address, AllocatedSize);
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

void* FMallocScaledPageMalloc::TryAllocateBlock(SIZE_T Size, SIZE_T& OutSize, void* AreaBaseAddr)
{
	void* Ptr = nullptr;
	if (AreaBaseAddr)
	{
		for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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
		for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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

void* FMallocScaledPageMalloc::TryAllocateBlock(void* Address, SIZE_T Size, SIZE_T& OutSize)
{

	void* Ptr = nullptr;
	for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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

bool FMallocScaledPageMalloc::FreeBlock(FMemoryBlock Block)
{
#if PAGE_MALLOC_STATS
	++Stats.FreeRequests;
#endif

	bool Ok = false;

	for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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

bool FMallocScaledPageMalloc::SetProtection(FMemoryBlock Block, FVMBlockAccess AccessFlag)
{
	bool bCanRead = false;
	bool bCanWrite = false;

	switch (AccessFlag)
	{
	case FVMBlockAccess::NoAccess:
		break;
	case FVMBlockAccess::ReadOnly:
		bCanRead = true;
		break;
	case FVMBlockAccess::ReadWrite:
		bCanRead = true;
		bCanWrite = true;
		break;
	case FVMBlockAccess::Execute:
		break;
	case FVMBlockAccess::FullAccess:
		break;
	default:
		break;
	}

	return FPlatformMemory::PageProtect(Block.GetBase(), Block.GetSize(), bCanRead, bCanWrite);
}

bool FMallocScaledPageMalloc::IsProtectionSupported()
{
	return true;
}

SIZE_T FMallocScaledPageMalloc::GetGranularity()
{
	return ArenaPageSize;
}

SIZE_T FMallocScaledPageMalloc::GetPageSize()
{
	return PageSize;
}

bool FMallocScaledPageMalloc::Free()
{
	bool Ok = false;
	for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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
		Stats = FPageMallocStats{};
	}
#endif

	return Ok;
}

bool FMallocScaledPageMalloc::Release(void* AreaBaseAddr)
{
	// not implemented yet;
	return true;
}

bool FMallocScaledPageMalloc::Release()
{

	bool Ok = false;
	for (SIZE_T i = 0; i < MALLOC_SCALED_MAX_ARENA_COUNT; ++i)
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
		Stats = FPageMallocStats{};
	}

#endif

	return Ok;
}


void FMallocScaledPageMalloc::GetStats(FPageMallocStats& OutStats)
{
#if PAGE_MALLOC_STATS
	Guard.Lock();
	OutStats = Stats;
	Guard.Unlock();
#else
	printf("PAGE MALLOC: INF: Page allocator stats are not enabled\n");
#endif
}

bool FMallocScaledPageMalloc::FArenaTable::Init(SIZE_T InArenaCount)
{
	SIZE_T DataBlockSize = Align(ArenaSlotSize * InArenaCount, 4096);

	void* BlkPtr = FPlatformMemory::BinnedAllocFromOS(DataBlockSize);

	if (BlkPtr)
	{
		DataBlock = { BlkPtr, DataBlockSize };

		FirstArena = (FArenaSlot*)DataBlock.GetBase();
		LastArena = FirstArena + (ArenaCount - 1);

		this->ArenaCount = InArenaCount;
		DefaultConstructItems<FArenaSlot>(FirstArena, InArenaCount);
		return true;
	}

	return false;
}


SIZE_T FMallocScaledPageMalloc::FArenaTable::GetArenaSlotIndex(FArenaSlot* Node)
{
	return Node - FirstArena;
}

SIZE_T FMallocScaledPageMalloc::FArenaTable::GetArenaSlotCount()
{
	return ArenaCount;
}

int32 FMallocScaledPageMalloc::FArenaTable::GetFreeSlot()
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

void FMallocScaledPageMalloc::FArenaTable::Free()
{
	for (int32 i = 0; i < ArenaCount; ++i)
	{
		FirstArena[i].Free = true;
	}
}

bool FMallocScaledPageMalloc::FArenaTable::Release()
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

void FMallocScaledPageMalloc::GetLastTimeStats(FPageMallocTimeStats& OutLastTimeStats)
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
SIZE_T FMallocScaledPageMalloc::GetArenaDefaultSize()
{
	return DEFAULT_ARENA_SIZE;
}

void FMallocScaledPageMalloc::DumpMemMap(void* AreaBaseAddr)
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

void FMallocScaledPageMalloc::FArena::DumpMemMap()
{
	FBlock* bfirst = ArenaPages.GetFirst();
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