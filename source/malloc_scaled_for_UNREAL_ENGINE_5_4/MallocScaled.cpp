// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/MallocScaled.h"
#include "Math/UnrealMathUtility.h"
#include "Templates/MemoryOps.h"

//UE_DISABLE_OPTIMIZATION

void TMallocScaledTimeStats::AddInterval(TMallocScaledDuration TimeInterval)
{
	LastTime = TimeInterval;
	++MeasureCount;

	if (TimeInterval > MaxTime)
		MaxTime = TimeInterval;

	if (TimeInterval < MinTime)
		MinTime = TimeInterval;

	TotalTime += TimeInterval;
	AvgTime = TotalTime / MeasureCount;
}

TMallocScaledTimeStats& TMallocScaledTimeStats::operator+(TMallocScaledDuration TimeInterval)
{
	AddInterval(TimeInterval);
	return *this;
}

TMallocScaledTimeStats& TMallocScaledTimeStats::operator+=(TMallocScaledDuration TimeInterval)
{
	return *this + TimeInterval;
}

TMallocScaledDuration TMallocScaledTimeStats::GetMaxTime()
{
	return MaxTime;
}

TMallocScaledDuration TMallocScaledTimeStats::GetMinTime()
{
	return MinTime;
}

TMallocScaledDuration TMallocScaledTimeStats::GetAvgTime()
{
	return AvgTime;
}

TMallocScaledDuration TMallocScaledTimeStats::GetLastTime()
{
	return LastTime;
}

TMallocScaledDuration TMallocScaledTimeStats::GetTotalTime()
{
	return TotalTime;
}

void TMallocScaledTimeStats::Reset()
{
	MaxTime = {};
	MinTime = {};
	AvgTime = {};
	LastTime = {};
	TotalTime = {};

	MeasureCount = 0;
}

//#define MAX_MALLOC_ALIGNMENT 64
static const SIZE_T MALLOC_SCALED_ALLOCATION_ADJUSTMENT   = 3;
static const SIZE_T MALLOC_SCALED_REALLOCATION_ADJUSTMENT = 4;

SIZE_T FMallocScaledImpl::TMemPoolTable::CalculateNumOfBaseEntries(SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize)
{
	return (FMallocScaledMath::FloorLog2(MaxBaseBlockSize) - FMallocScaledMath::FloorLog2(MinBaseBlockSize) + 1);
}

SIZE_T FMallocScaledImpl::TMemPoolTable::CalculatePoolBlockSize(SIZE_T BaseIndex, SIZE_T PoolIndex, SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize, uint32 SubIndexCount)
{
	SIZE_T MinBaseIndex = FMallocScaledMath::Log2_64(MinBaseBlockSize);
	SIZE_T MaxBaseIndex = FMallocScaledMath::Log2_64(MaxBaseBlockSize);

	SIZE_T BlockIndex = MinBaseIndex + BaseIndex;

	if (BlockIndex > MaxBaseIndex)
	{
		return 0;
	}

	SIZE_T UpperPoolBlockSize = FMallocScaledMath::Pow2_64(BlockIndex);
	SIZE_T LowerPoolBlockSize = BlockIndex == MinBaseIndex ? 0 : FMallocScaledMath::Pow2_64(BlockIndex - 1);

	SIZE_T Spacing = (UpperPoolBlockSize - LowerPoolBlockSize) / SubIndexCount;
	SIZE_T BlockSize = LowerPoolBlockSize + Spacing * (PoolIndex + 1);

	return BlockSize;
}

bool FMallocScaledImpl::TMemPoolTable::GetBaseIndex(SIZE_T BlockSize, SIZE_T MinBaseIndex, SIZE_T MaxBaseIndex, SIZE_T& OutBaseIdx)
{
	SIZE_T BlockIndex = FMallocScaledMath::CeilLog2(BlockSize);
	if (BlockIndex < MinBaseIndex)
	{
		OutBaseIdx = 0;
		return true;
	}

	SIZE_T BaseIndex = BlockIndex - MinBaseIndex;

	if (BaseIndex > MaxBaseIndex - MinBaseIndex)
	{
		return false;
	}

	OutBaseIdx = BaseIndex;
	return true;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetPoolIndex(SIZE_T BlockSize, SIZE_T BaseIndex, SIZE_T MinBaseIndex, SIZE_T SubIndexCount)
{
	SIZE_T BlockIndex = MinBaseIndex + BaseIndex;
	SIZE_T UpperPoolBlockSize = FMallocScaledMath::Pow2_64(BlockIndex);
	SIZE_T LowerPoolBlockSize = BlockIndex == MinBaseIndex ? 0 : FMallocScaledMath::Pow2_64(BlockIndex - 1);

	SIZE_T Spacing = (UpperPoolBlockSize - LowerPoolBlockSize) / SubIndexCount;
	SIZE_T PoolIndex = ((BlockSize - LowerPoolBlockSize) / Spacing);

	if (PoolIndex == SubIndexCount)
	{
		--PoolIndex;
	}

	return PoolIndex;
}

void FMallocScaledImpl::TMemPool::Init(SIZE_T InBaseIndex, SIZE_T InPoolIndex, SIZE_T InBlockSize, SIZE_T InPoolBlockSize)
{
	this->PoolIndex = InPoolIndex;
	this->BaseIndex = InBaseIndex;
	this->BlockSize = InBlockSize;
	this->PoolBlockSize = InPoolBlockSize;
	this->PoolVMBlockSize = Align(InPoolBlockSize + MemPoolHdrSize, TMallocScaledVMBlock::GetPageSize());

#if MALLOC_SCALED_MALLOC_STATS
	Stats.BlockSize = BlockSize;
#endif
}

FMallocScaledImpl::TMemPoolHdr* FMallocScaledImpl::TMemPool::AddPool()
{
	TMallocScaledVMBlock NewPoolVMBlock;

	if (BlockSize > PoolBlockSize)
	{
		PoolVMBlockSize = Align(MemPoolHdrSize + MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize, TMallocScaledVMBlock::GetPageSize());

#if MALLOC_SCALED_MALLOC_DEBUG
		printf("MALLOC: DBG: ADDING NEW MEM POOL; BIG BLOCK SIZE, ALLOCATED SIZE: %llu\n", PoolVMBlockSize);
#endif
	}
	else
	{
#if MALLOC_SCALED_MALLOC_DEBUG
		printf("MALLOC: DBG: ADDING NEW MEM POOL; PRECOMMITED SIZE: %llu, ALLOCATED SIZE: %llu\n", PoolBlockSize, PoolVMBlockSize);
#endif
	}

	bool Ok = NewPoolVMBlock.Allocate(PoolVMBlockSize);

	if (!Ok)
	{
		return nullptr;
	}

	Ok = NewPoolVMBlock.Commit(NewPoolVMBlock.GetBase(), NewPoolVMBlock.GetAllocatedSize());

	if (!Ok)
	{
		NewPoolVMBlock.Deallocate();
		return nullptr;
	}

	PoolBlockSize = PoolVMBlockSize - MemPoolHdrSize;

	TMemPoolHdr* PoolHdr = (TMemPoolHdr*)NewPoolVMBlock.GetBase();
	DefaultConstructItems<TMemPoolHdr>(PoolHdr, 1);

	PoolHdr->MemPool = this;
	PoolHdr->PoolVMBlock = std::move(NewPoolVMBlock);
	SIZE_T BlockCount = PoolBlockSize / (MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize);
	PoolHdr->TotalBlockCount = BlockCount;
	PoolHdr->FreeBlockCount = BlockCount;

	++PoolCount;
	PoolList.PushBack(PoolHdr);

#if MALLOC_SCALED_MALLOC_STATS
	Stats.AllocatedSize += PoolVMBlockSize;
	Stats.TotalBlockCount += BlockCount;
	Stats.FreeBlockCount += BlockCount;
#endif

	return PoolHdr;
}

void FMallocScaledImpl::TMemPool::DeletePool(TMemPoolHdr* Pool)
{
	PoolList.Delete(Pool);
	--PoolCount;

#if MALLOC_SCALED_MALLOC_STATS
	Stats.AllocatedSize -= PoolVMBlockSize;
	Stats.TotalBlockCount -= Pool->TotalBlockCount;
	Stats.UsedBlockCount -= Pool->TotalBlockCount - Pool->FreeBlockCount;
	Stats.FreeBlockCount -= Pool->FreeBlockCount;
	Stats.Used -= Pool->Used;
#endif
	// !!! we move PoolVMBlock to tmp place because it sits on mem area which will be destroyed by Deallocate();
	TMallocScaledVMBlock PoolVMBlock = std::move(Pool->PoolVMBlock);
	PoolVMBlock.Deallocate();
}

FMallocScaledImpl::TMemPoolHdr* FMallocScaledImpl::TMemPool::FindFreePool()
{
	auto PoolNode = PoolList.GetFirst();

	if (PoolNode)
	{
		auto Pool = *PoolNode->GetElement();

		while (PoolNode)
		{
			if (Pool->FreeBlockCount)
			{
				return Pool;
			}

			Pool = *PoolNode->GetElement();
			PoolNode = PoolNode->GetNext();
		}
	}

	return nullptr;
}


FMallocScaledImpl::TMemBlockHdr* FMallocScaledImpl::TMemPool::GetFreeBlock(SIZE_T AdjustedUsedSize, SIZE_T UsedSize)
{
	auto PoolNode = FindFreePool();

	if (!PoolNode)
	{
		PoolNode = AddPool();

		if (!PoolNode)
		{
#if MALLOC_SCALED_MALLOC_DEBUG
			printf("MALLOC: DBG: ERROR: OUT OF MEMORY: Cannot add memory pool\n");
#endif
			return nullptr;
		}
	}

	TMemBlockHdr* FreeBlock = nullptr;

	auto Pool = *PoolNode->GetElement();
	auto FoundBlock = Pool->FreeBlockList.PopBack();

	if (!FoundBlock)
	{
		if (Pool->ActiveBlocks != Pool->TotalBlockCount)
		{
			uint8* FirstBlock = (uint8*)(Pool + 1);
			auto InactiveBlock = (FirstBlock + (MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize) * Pool->ActiveBlocks);
			++Pool->ActiveBlocks;
			FreeBlock = (TMemBlockHdr*)InactiveBlock;
			DefaultConstructItems<TMemBlockHdr>(FreeBlock, 1);

			FreeBlock->BlockSize = BlockSize;
			FreeBlock->PoolHdr = Pool;
		}
		else
		{
#if MALLOC_SCALED_MALLOC_DEBUG
			printf("MALLOC: DBG: Memory pool %p is corrupted\n", this);
#endif
		}
	}
	else
	{
		FreeBlock = *FoundBlock->GetElement();
	}

	if (FreeBlock)
	{
		FreeBlock->UsedSize = UsedSize;
		--Pool->FreeBlockCount;

#if MALLOC_SCALED_MALLOC_STATS
		Pool->Used += UsedSize;
		Stats.Used += UsedSize;

		if (UsedSize > Stats.BlockStats.LargestUsedBlock)
		{
			Stats.BlockStats.LargestUsedBlock = UsedSize;
		}

		if (UsedSize < Stats.BlockStats.SmallestUsedBlock)
		{
			Stats.BlockStats.SmallestUsedBlock = UsedSize;
		}

		if (Stats.Used > Stats.PeakUsed)
		{
			Stats.PeakUsed = Stats.Used;
		}

		++Stats.UsedBlockCount;

		if (Stats.UsedBlockCount > Stats.PeakUsedBlockCount)
		{
			Stats.PeakUsedBlockCount = Stats.UsedBlockCount;
		}

#endif
#if MALLOC_SCALED_MALLOC_STATS
		Pool->UsrBlockList.PushBack(FreeBlock);
#endif
	}

	return FreeBlock;
}

void FMallocScaledImpl::TMemPool::Free()
{
	auto NextPoolNode = PoolList.GetFirst();
	bool Ok = true;

	while (NextPoolNode)
	{
		auto PoolToFree = *NextPoolNode->GetElement();
		NextPoolNode = NextPoolNode->GetNext();
		DeletePool(PoolToFree);
	}

#if MALLOC_SCALED_MALLOC_STATS
	Stats = {};
#endif
}

void FMallocScaledImpl::TMemPool::FreeUsrBlock(TMemBlockHdr* UsrBlock)
{
	if (UsrBlock)
	{
		auto Pool = UsrBlock->PoolHdr;
		SIZE_T UsedSize = UsrBlock->UsedSize;

#if MALLOC_SCALED_MALLOC_STATS
		Pool->UsrBlockList.Delete(UsrBlock);
		Pool->Used -= UsrBlock->UsedSize;
#endif
		Pool->FreeBlockList.PushBack(UsrBlock);
		++Pool->FreeBlockCount;

#if MALLOC_SCALED_MALLOC_STATS
		Stats.Used -= UsrBlock->UsedSize;
		--Stats.UsedBlockCount;
#endif
		UsrBlock->UsedSize = 0;
	}
}

SIZE_T FMallocScaledImpl::TMemPool::GetBlockSize()
{
	return BlockSize;
}

SIZE_T FMallocScaledImpl::TMemPool::GetPoolCount()
{
	return PoolCount;
}

FMallocScaledImpl::TMemPool::TMemPoolStats* FMallocScaledImpl::TMemPool::GetPoolStats()
{
#if MALLOC_SCALED_MALLOC_STATS
	auto FirstPool = PoolList.GetFirst();

	for (auto Pool = FirstPool; Pool != nullptr; Pool = Pool->GetNext())
	{
		auto PoolHdr = *(Pool->GetElement());
		auto FirstBlock = PoolHdr->UsrBlockList.GetFirst();

		for (auto Block = FirstBlock; Block != nullptr; Block = Block->GetNext())
		{
			auto BlockHdr = *(Block->GetElement());

			if (BlockHdr->UsedSize > Stats.BlockStats.MaxUsedBlock)
			{
				Stats.BlockStats.MaxUsedBlock = BlockHdr->UsedSize;
			}

			if (BlockHdr->UsedSize < Stats.BlockStats.MinUsedBlock)
			{
				Stats.BlockStats.MinUsedBlock = BlockHdr->UsedSize;
			}
		}
	}

	return &Stats;
#elif MALLOC_SCALED_MALLOC_DEBUG
	printf("MALLOC: DBG: Malloc stats are not enabled");
	return nullptr;
#else
	return nullptr;
#endif


}

bool FMallocScaledImpl::TMemPoolTableEntry::Init(SIZE_T InBaseIndex, SIZE_T InMinBaseBlockSize, SIZE_T InMaxBaseBlockSize, SIZE_T InPoolBlockSize, uint32 InSubIndexCount)
{
	if (Initialized)
	{
		return false;
	}

	if (InSubIndexCount > MaxSubIndexCount)
	{
		// log error;
		return false;
	}

	for (SIZE_T i = 0; i < InSubIndexCount; ++i)
	{
		SIZE_T BlockSize = TMemPoolTable::CalculatePoolBlockSize(InBaseIndex, i, InMinBaseBlockSize, InMaxBaseBlockSize, InSubIndexCount);
		Pools[i].Init(InBaseIndex, i, BlockSize, InPoolBlockSize);
	}

	this->SubIndexCount = InSubIndexCount;
	Initialized = true;
	return true;
}

void FMallocScaledImpl::TMemPoolTableEntry::Free()
{
	for (SIZE_T i = 0; i < SubIndexCount; ++i)
	{
		Pools[i].Free();
	}

	Initialized = false;
}

SIZE_T FMallocScaledImpl::TMemPoolTableEntry::GetPoolCount()
{
	return SubIndexCount;
}

FMallocScaledImpl::TMemPool* FMallocScaledImpl::TMemPoolTableEntry::GetPool(SIZE_T PoolIndex)
{
	return &Pools[PoolIndex];
}

bool FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::Init(SIZE_T InEntryCount)
{
	bool Ok = false;
	SIZE_T DataBlockSize = Align(EntrySize * InEntryCount, TMallocScaledVMBlock::GetPageSize());

	Ok = DataBlock.Allocate(DataBlockSize);

	if (!Ok)
	{
		return false;
	}

	Ok = DataBlock.Commit();

	if (!Ok)
	{
		return false;
	}

	FirstEntry = (TMemPoolTableEntry*)DataBlock.GetBase();
	LastEntry = FirstEntry + (InEntryCount - 1);
	this->EntryCount = InEntryCount;
	DefaultConstructItems<TMemPoolTableEntry>(FirstEntry, InEntryCount);

	return Ok;
}

FMallocScaledImpl::TMemPoolTableEntry* FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::GetFirst()
{
	return FirstEntry;
}

FMallocScaledImpl::TMemPoolTableEntry* FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::GetLast()
{
	return LastEntry;
}

FMallocScaledImpl::TMemPoolTableEntry* FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::GetEntry(SIZE_T Index)
{
	if (Index < EntryCount)
		return FirstEntry + Index;

	return nullptr;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::GetEntryIndex(TMemPoolTableEntry* Entry)
{
	return Entry - FirstEntry;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::GetEntryCount()
{
	return EntryCount;
}

void FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::Free()
{
	DefaultConstructItems<TMemPoolTableEntry>(FirstEntry, EntryCount);
}

void FMallocScaledImpl::TMemPoolTable::TMemPoolTableStorage::Release()
{
	DataBlock.Deallocate();

	FirstEntry = nullptr;
	LastEntry = nullptr;
	EntryCount = 0;
	DataBlock = {};
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetPoolBlockSize()
{
	return PoolBlockSize;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetEntryCount()
{
	return BaseEntries.GetEntryCount();
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetSubIndexCount()
{
	return SubIndexCount;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetMinBaseBlockSize()
{
	return MinBaseBlockSize;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetMaxBaseBlockSize()
{
	return MaxBaseBlockSize;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetMinBaseIndex()
{
	return MinBaseIndex;
}

SIZE_T FMallocScaledImpl::TMemPoolTable::GetMaxBaseIndex()
{
	return MaxBaseIndex;
}

FMallocScaledImpl::TMemPoolTableEntry* FMallocScaledImpl::TMemPoolTable::GetEntry(SIZE_T EntryNum)
{
	return &BaseEntries[EntryNum];
}

bool FMallocScaledImpl::TMemPoolTable::Init(SIZE_T InMinBaseBlockSize, SIZE_T InMaxBaseBlockSize, SIZE_T InPoolBlockSize, uint32 InSubIndexCount)
{
#if	MALLOC_SCALED_MALLOC_DEBUG
	if (!IsPow2(MinBaseBlockSize))
	{
		printf("MALLOC: DBG: ERROR: MinBaseBlocksize is not power of 2 corrupted\n");
		return false;
	}

	if (!IsPow2(MaxBaseBlockSize))
	{
		printf("MALLOC: DBG: ERROR: MaxBaseBlocksize is not power of 2 corrupted\n");
		return false;
	}

	if (PoolBlockSize % DEFAULT_ALIGNMENT)
	{
		printf("MALLOC: DBG: ERROR: PoolBlockSize is not aligned by DEFAULT_ALIGNMENT\n");
		return false;
	}

	if (!IsPow2(SubIndexCount))
	{
		printf("MALLOC: DBG: ERROR: SubIndexCount is not power of 2 corrupted\n");
		return false;
	}
#endif

	bool Ok = false;

	SIZE_T EntryCount = CalculateNumOfBaseEntries(InMinBaseBlockSize, InMaxBaseBlockSize);

	Ok = BaseEntries.Init(EntryCount);

	if (Ok)
	{
		for (uint32 i = 0; i < EntryCount; ++i)
		{
			BaseEntries[i].Init(i, InMinBaseBlockSize, InMaxBaseBlockSize, InPoolBlockSize, InSubIndexCount);

			if (!Ok)
			{
				return false;
			}
		}

		this->PoolBlockSize    = InPoolBlockSize;
		this->MaxBaseBlockSize = InMaxBaseBlockSize;
		this->MaxBaseIndex     = FMallocScaledMath::Log2_64(InMaxBaseBlockSize);
		this->MinBaseBlockSize = InMinBaseBlockSize;
		this->MinBaseIndex     = FMallocScaledMath::Log2_64(InMinBaseBlockSize);
		this->SubIndexCount    = InSubIndexCount;

		return true;
	}

	return false;
}

void FMallocScaledImpl::TMemPoolTable::Free()
{
	SIZE_T EntryCount = BaseEntries.GetEntryCount();

	for (uint32 i = 0; i < EntryCount; ++i)
	{
		BaseEntries[i].Free();
	}

	BaseEntries.Release();
}

void FMallocScaledImpl::TMemPoolTable::UpdateStats()
{
#if MALLOC_SCALED_MALLOC_STATS
	SIZE_T EntryCount = BaseEntries.GetEntryCount();

	for (SIZE_T i = 0; i < EntryCount; ++i)
	{
		SIZE_T PoolCount = BaseEntries[i].GetPoolCount();
		for (SIZE_T j = 0; j < PoolCount; ++j)
		{
			TMemPool* Pool = BaseEntries[i].GetPool(j);
			TMemPool::TMemPoolStats* PoolStats = Pool->GetPoolStats();

			Stats.Used += PoolStats->Used;
			Stats.UsedBlockCount += PoolStats->UsedBlockCount;
			Stats.TotalBlockCount += PoolStats->TotalBlockCount;
			Stats.AllocatedSize += PoolStats->AllocatedSize;

			if (PoolStats->PeakUsed > Stats.PeakUsed)
			{
				Stats.PeakUsed = PoolStats->PeakUsed;
			}

			if (PoolStats->PeakUsedBlockCount > Stats.PeakUsedBlockCount)
			{
				Stats.PeakUsedBlockCount = PoolStats->PeakUsedBlockCount;
			}
		}
	}

#elif  MALLOC_SCALED_MALLOC_DEBUG
	printf("MALLOC: DBG: Malloc stats are not enabled");
#endif
}


void* FMallocScaledImpl::MallocInternal(SIZE_T Size, uint32 Alignment)
{
#if MALLOC_SCALED_MALLOC_DEBUG
	printf("\nMALLOC: DBG: ALLOCATE MEM BLOCK: Size: %llu\n", Size);
#endif

#if MALLOC_SCALED_MALLOC_STATS
	++MallocStats.RequestStats.MallocRequests;

	if (Size > MallocStats.RequestVals.MaxAllocRequestSize)
	{
		MallocStats.RequestVals.MaxAllocRequestSize = Size;
	}

	if (Size < MallocStats.RequestVals.MinAllocRequestSize)
	{
		MallocStats.RequestVals.MinAllocRequestSize = Size;
	}
#endif

#if MALLOC_SCALED_MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif

	void* UsrBlockPtr = nullptr;

	if (Size && 
		FMallocScaledMath::IsPow2(Alignment) && 
		Alignment <= TMallocScaledVMBlock::GetPageSize())
	{
		SIZE_T AdjustedBlockSize = Size + (Size >> MALLOC_SCALED_ALLOCATION_ADJUSTMENT);
		AdjustedBlockSize = Align(AdjustedBlockSize + Alignment, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT);
		SIZE_T BaseIndex;
		bool Ok = TMemPoolTable::GetBaseIndex(AdjustedBlockSize, PoolTable.GetMinBaseIndex(), PoolTable.GetMaxBaseIndex(), BaseIndex);

		if (Ok)
		{
			SIZE_T PoolIdx = TMemPoolTable::GetPoolIndex(AdjustedBlockSize, BaseIndex, PoolTable.GetMinBaseIndex(), PoolTable.GetSubIndexCount());
			TMemPoolTableEntry* BaseEntry = PoolTable.GetEntry(BaseIndex);

			TMemPool* Pool = BaseEntry->GetPool(PoolIdx);
			TMemBlockHdr* FreeBlock = Pool->GetFreeBlock(AdjustedBlockSize, Size);

			if (FreeBlock)
			{
				UsrBlockPtr = FreeBlock + 1;
				void* AlignedPtr = UsrBlockPtr;

				if (Alignment)
				{
					AlignedPtr = Align(UsrBlockPtr, Alignment);
				}
				
				if (AlignedPtr == UsrBlockPtr)
				{
					UsrBlockPtr = (TMemBlockHdrOffset*)AlignedPtr + 1;
					TMemBlockHdrOffset* HdrOffset = (TMemBlockHdrOffset*)AlignedPtr;
					HdrOffset->BlockHdr = FreeBlock;
				}
				else
				{
					UsrBlockPtr = AlignedPtr;
					TMemBlockHdrOffset* HdrOffset = (TMemBlockHdrOffset*)AlignedPtr - 1;
					HdrOffset->BlockHdr = FreeBlock;
				}
			}
		}
	}

#if MALLOC_SCALED_MALLOC_TIME_STATS
	Timer.Stop();
	MallocTimeStats.BlockAllocTime += Timer.GetDuration();
	std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
	printf("MALLOC: DBG: Allocation block Size: %llu Address: 0x%p Time: %f ms\n", Size, UsrBlockPtr, d.count());
#endif

#if MALLOC_SCALED_MALLOC_DEBUG
	if (UsrBlockPtr)
	{
		printf("MALLOC: DBG: ALLOCATE MEM BLOCK -[OK]: Size: %llu at address %p\n", Size, UsrBlockPtr);
	}
	else
	{
		printf("MALLOC: DBG: ALLOCATE MEM BLOCK -[FAILED]: Size: %llu at address %p - \n", Size, UsrBlockPtr);
	}
#endif

#if MALLOC_SCALED_MALLOC_STATS
	if (UsrBlockPtr)
	{
		MallocStats.TotalStats.TotalUsed += Size; // += AdjustedBlockSize; // 

		if (MallocStats.TotalStats.TotalUsed > MallocStats.TotalStats.MaxTotalUsed)
		{
			MallocStats.TotalStats.MaxTotalUsed = MallocStats.TotalStats.TotalUsed;
		}
		else
		{
			++MallocStats.RequestStats.FailedMallocRequests;
		}
	}
#endif

	return UsrBlockPtr;
}

void* FMallocScaledImpl::ReallocInternal(void* Addr, SIZE_T NewSize, uint32 NewAlignment)
{
	SIZE_T OldSize = GetSizeInternal(Addr);
#if MALLOC_SCALED_MALLOC_DEBUG
	printf("\nMALLOC: DBG: REALLOCATE MEM BLOCK: Address: 0x%p, Old size: %llu, New size: %llu\n", Addr, OldSize, Size);
#endif

#if MALLOC_SCALED_MALLOC_STATS
	++MallocStats.RequestStats.ReallocRequests;
#endif

#if MALLOC_SCALED_MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif

	if (!NewAlignment)
	{
		NewAlignment = MALLOC_SCALED_DEFAULT_ALIGNMENT;
	}

	void* UsrBlockPtr = nullptr;
	if (NewSize && 
		Addr && 
		FMallocScaledMath::IsPow2(NewAlignment) && 
		NewAlignment <= TMallocScaledVMBlock::GetPageSize())
	{
		void* NewPtr = nullptr;
		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;

		SIZE_T AdjustedNewSize = NewSize + (NewSize >> MALLOC_SCALED_ALLOCATION_ADJUSTMENT);
		AdjustedNewSize = Align(AdjustedNewSize + NewAlignment, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT);

		if (AdjustedNewSize > Block->BlockSize || AdjustedNewSize < (Block->BlockSize >> MALLOC_SCALED_REALLOCATION_ADJUSTMENT))
		{
			NewPtr = MallocInternal(NewSize, NewAlignment);
		}
		else
		{
			OldSize = Block->UsedSize;
			Block->UsedSize = NewSize;
			void* AlignedPtr = Addr;

			//if (NewAlignment)
			{
				AlignedPtr = Align(Addr, NewAlignment);
			}

			if (AlignedPtr == Addr)
			{
				UsrBlockPtr = Addr;
			}
			else
			{
				UsrBlockPtr = AlignedPtr;
				TMemBlockHdrOffset* HdrOffset = (TMemBlockHdrOffset*)AlignedPtr - 1;
				HdrOffset->BlockHdr = Block;
			}
		}

		if (NewPtr)
		{

#if MALLOC_SCALED_MALLOC_STATS
			++MallocStats.RequestStats.FailedReallocInPlaceRequests;
#endif
			SIZE_T SizeToCopy = NewSize < OldSize ? NewSize : OldSize;
			memcpy(NewPtr, Addr, SizeToCopy);
			FreeInternal(Addr);
			UsrBlockPtr = NewPtr;
		}

#if MALLOC_SCALED_MALLOC_TIME_STATS
		Timer.Stop();
		MallocTimeStats.BlockReallocTime += Timer.GetDuration();
		std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
		printf("MALLOC: DBG: Reallocation block Old size: %llu Address: 0x%p; New size: %llu New address: 0x%p Time : %f ms\n", OldSize, Addr, Size, UsrBlockPtr, d.count());
#endif

#if MALLOC_SCALED_MALLOC_DEBUG
		if (UsrBlockPtr)
		{
			printf("MALLOC: DBG: REALLOCATE MEM BLOCK -[OK]: Old size: %llu at address %p; New size %llu at new address %p - \n", OldSize, Addr, Size, UsrBlockPtr);
		}
		else
		{
			printf("MALLOC: DBG: REALLOCATE MEM BLOCK -[FAILED]: Old size: %llu at address %p - \n", Size, Addr);
		}

#endif

#if MALLOC_SCALED_MALLOC_STATS
		if (UsrBlockPtr)
		{
			if (!NewPtr)
			{
				MallocStats.TotalStats.TotalUsed -= OldSize;
				MallocStats.TotalStats.TotalUsed += Size;

				if (MallocStats.TotalStats.TotalUsed > MallocStats.TotalStats.MaxTotalUsed)
				{
					MallocStats.TotalStats.MaxTotalUsed = MallocStats.TotalStats.TotalUsed;

				}
			}
		}
		else
		{
			++MallocStats.RequestStats.FailedReallocRequests;
		}

#endif
	}

	else if (Addr == nullptr)
	{
		UsrBlockPtr = MallocInternal(NewSize, NewAlignment);
	}
	else
	{
		Free(Addr);
		UsrBlockPtr = nullptr;
	}

	return UsrBlockPtr;
}

void FMallocScaledImpl::FreeInternal(void* Addr)
{
#if MALLOC_SCALED_MALLOC_DEBUG
	SIZE_T Size0 = GetSizeInternal(Addr);
	printf("\nMALLOC: DBG: FREE MEM BLOCK: Address: 0x%p, Size: %llu\n", Addr, Size0);
#endif

#if MALLOC_SCALED_MALLOC_STATS
	SIZE_T Size1 = GetSizeInternal(Addr);

	++MallocStats.RequestStats.FreeRequests;
	if (Size1 > MallocStats.RequestVals.MaxFreeRequestSize)
	{
		MallocStats.RequestVals.MaxFreeRequestSize = Size1;
	}

	if (Size1 < MallocStats.RequestVals.MinFreeRequestSize)
	{
		MallocStats.RequestVals.MinFreeRequestSize = Size1;
	}

#endif

#if MALLOC_SCALED_MALLOC_TIME_STATS
	TTimer Timer;
	SIZE_T Size2 = GetSizeInternal(Addr);
	Timer.Start();
#endif

	bool Ok = false;
	if (Addr)
	{
		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;
		TMemPoolHdr* PoolHdr = Block->PoolHdr;
		PoolHdr->MemPool->FreeUsrBlock(Block);

#if MALLOC_SCALED_MALLOC_TIME_STATS
		Timer.Stop();
		MallocTimeStats.BlockFreeTime += Timer.GetDuration();
		std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
		printf("MALLOC: DBG: Free block Address: 0x%p, Size: %llu Time : %f ms\n", Addr, Size2, d.count());
#endif

#if MALLOC_SCALED_MALLOC_DEBUG
		if (Ok)
		{
			printf("MALLOC: DBG: FREE MEM BLOCK -[OK]: Address: 0x%p, Size: %llu\n", Addr, Size0);
		}
		else
		{
			printf("MALLOC: DBG: FREE MEM BLOCK -[FAILED]: Address: 0x%p, Size: %llu\n", Addr, Size0);
		}
#endif

#if MALLOC_SCALED_MALLOC_STATS
		if (Ok)
		{
			//SIZE_T Size = GetSize(Addr);
			MallocStats.TotalStats.TotalUsed -= Size1;
		}
		else
		{
			++MallocStats.RequestStats.FailedFreeRequests;
		}
#endif
	}

}

SIZE_T FMallocScaledImpl::GetSizeInternal(void* Addr)
{
	SIZE_T UsedSize = 0;
	if (Addr)
	{
		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;
		UsedSize = Block->UsedSize;
	}

	return UsedSize;
}

void* FMallocScaledImpl::Malloc(SIZE_T Size, uint32 Alignment)
{
	CriticalSection.Lock();
	void* FreeBlock = MallocInternal(Size, Alignment);
	CriticalSection.Unlock();
	return FreeBlock;
}

void* FMallocScaledImpl::Realloc(void* Addr, SIZE_T NewSize, uint32 NewAlignment)
{
	CriticalSection.Lock();
	void* ReallocatedBlock = ReallocInternal(Addr, NewSize, NewAlignment);
	CriticalSection.Unlock();
	return ReallocatedBlock;
}

void  FMallocScaledImpl::Free(void* Addr)
{
	CriticalSection.Lock();
	FreeInternal(Addr);
	CriticalSection.Unlock();
}

SIZE_T FMallocScaledImpl::GetSize(void* Addr)
{
	CriticalSection.Lock();
	SIZE_T UsedSize = GetSizeInternal(Addr);
	CriticalSection.Unlock();
	return UsedSize;
}

void FMallocScaledImpl::DebugInit(SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize, SIZE_T PoolBlockSize, uint32 SubIndexCount)
{
#if MALLOC_SCALED_MALLOC_DEBUG
	bool Ok = false;

	if (!Initialized)
	{
		Ok = TVMBlock::Init();
		printf("MALLOC: DBG: Initializing memory allocator...\n");
		if (Ok)
		{
			Ok = Table.Init(MinBaseBlockSize, MaxBaseBlockSize, PoolBlockSize, SubIndexCount);

			if (Ok)
			{
				Initialized = true;
			}
		}
	}
	else
	{
		printf("MALLOC: DBG: Memory allocator is initialized already...\n");
	}
#endif
}

bool FMallocScaledImpl::Init()
{
	bool Ok = false;

	if (!Initialized)
	{
		Ok = TMallocScaledVMBlock::Init();
#if MALLOC_SCALED_MALLOC_DEBUG
		printf("MALLOC: DBG: Initializing memory allocator...\n");
#endif
		if (Ok)
		{
			Ok = PoolTable.Init(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE,
				MALLOC_SCALED_MAX_BASE_BLOCK_SIZE,
				MALLOC_SCALED_POOL_BLOCK_SIZE,
				MALLOC_SCALED_SUBINDEX_COUNT);

			if (Ok)
			{
				Initialized = true;
				return true;
			}
		}
	}
	else
	{
#if MALLOC_SCALED_MALLOC_DEBUG
		printf("MALLOC: DBG: Memory allocator is initialized already...\n");
#endif
	}

	return false;
}

bool FMallocScaledImpl::IsInitialized()
{
	return Initialized;
}


void FMallocScaledImpl::Destroy()
{
#if MALLOC_SCALED_MALLOC_DEBUG
	printf("MALLOC: DBG: Destroying memory allocator\n");
#endif

	PoolTable.Free();

#if MALLOC_SCALED_MALLOC_STATS
	MallocStats = {};
	MallocTimeStats = {};
#endif

	Initialized = false;

#if MALLOC_SCALED_MALLOC_DEBUG
	printf("MALLOC: DBG: Destroying memory allocator - Ok\n");
#endif
}

SIZE_T FMallocScaledImpl::GetMallocMaxAlignment()
{
	return TMallocScaledVMBlock::GetPageSize();
}

void FMallocScaledImpl::GetSpecificStats(void* OutStatData)
{
	//#if MALLOC_SCALED_MALLOC_STATS
	//	TBigMemBlock* FBlock = *(BigBlockMemRegion.BigBlocks.GetFirst()->GetElement());
	//	TBigMemBlock* LBlock = *(BigBlockMemRegion.BigBlocks.GetLast()->GetElement());
	//
	//	TBigMemBlock* Max;
	//	TBigMemBlock* Min;
	//	SearchMaxMin(FBlock, LBlock, Max, Min);
	//
	//	Stats.BigBlockStats.BlockStats.MaxUsedBlock = Max->UsedSize;
	//	Stats.BigBlockStats.BlockStats.MinUsedBlock = Min->UsedSize;
	//
	//	for (uint32 i = 0; i < TPooledMemRegion::MemPoolMaxIndex; ++i)
	//	{
	//		PooledMemRegion.MemPools[i].GetPoolStats(Stats.PoolStats[i]);
	//	}
	//	*((TPoolMallocStats*)OutStatData) = Stats;
	//#else
	//#if MALLOC_SCALED_MALLOC_DEBUG
	//	printf("MALLOC: DBG: Malloc stats are not enabled");
	//#endif
	//#endif
}

SIZE_T FMallocScaledImpl::GetBaseEntryCount()
{
	return PoolTable.GetEntryCount();
}

SIZE_T FMallocScaledImpl::GetBlockSize(SIZE_T BaseIndex, SIZE_T PoolIndex)
{
#if MALLOC_SCALED_MALLOC_DEBUG
	TMemPoolTableEntry* BaseEntry = Table.GetEntry(BaseIndex);

	TMemPool* Pool = BaseEntry->GetPool(PoolIndex);

	return Pool->GetBlockSize();
#else
	return 0;
#endif
}

SIZE_T FMallocScaledImpl::GetBlockCount(SIZE_T BaseIndex, SIZE_T PoolIndex)
{
#if MALLOC_SCALED_MALLOC_DEBUG
	TMemPoolTableEntry* BaseEntry = Table.GetEntry(BaseIndex);

	TMemPool* Pool = BaseEntry->GetPool(PoolIndex);

	SIZE_T BlockCount = Table.GetPoolBlockSize() / (Pool->GetBlockSize() + MemBlockHdrSize);

	return BlockCount;
#else
	return 0;
#endif
}

FMallocScaled::FMallocScaled()
{
	if (!Impl.Init())
	{
		// log;
	}
}

void* FMallocScaled::Malloc(SIZE_T Size, uint32 Alignment)
{
	return Impl.Malloc(Size, Alignment);
}

void* FMallocScaled::TryMalloc(SIZE_T Size, uint32 Alignment)
{
	return Impl.Malloc(Size, Alignment);
}

void* FMallocScaled::Realloc(void* Ptr, SIZE_T NewSize, uint32 NewAlignment)
{
	return Impl.Realloc(Ptr, NewSize, NewAlignment);
}

void* FMallocScaled::TryRealloc(void* Ptr, SIZE_T NewSize, uint32 NewAlignment)
{
	return Impl.Realloc(Ptr, NewSize, NewAlignment);
}

void FMallocScaled::Free(void* Ptr)
{
	return Impl.Free(Ptr);
}

bool FMallocScaled::GetAllocationSize(void* Original, SIZE_T& SizeOut)
{
	SizeOut = Impl.GetSize(Original);
	if (!SizeOut)
	{
		return false;
	}

	return true;
}

bool FMallocScaled::IsInternallyThreadSafe() const
{
	return true;
}

bool FMallocScaled::ValidateHeap()
{
	return true;
}