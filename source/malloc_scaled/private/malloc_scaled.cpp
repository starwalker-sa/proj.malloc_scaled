
#include "malloc_scaled.h"
#include "lib_malloc.h"
#include "align.h"
#include "element_build.h"
#include "search_min_max.h"
#include "timer.h"

static const TSize MALLOC_SCALED_ALLOCATION_ADJUSTMENT   = 3;
static const TSize MALLOC_SCALED_REALLOCATION_ADJUSTMENT = 4;

TTimeStats Ts;

TSize TMemPoolTable::CalculateNumOfBaseEntries(TSize MinBaseBlockSize, TSize MaxBaseBlockSize)
{
	return (FloorLog2(MaxBaseBlockSize) - FloorLog2(MinBaseBlockSize) + 1);
}

TSize TMemPoolTable::CalculatePoolBlockSize(TSize BaseIndex, TSize PoolIndex, TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize SubIndexCount)
{
	TSize MinBaseIndex = Log2_64(MinBaseBlockSize);
	TSize MaxBaseIndex = Log2_64(MaxBaseBlockSize);

	TSize BlockIndex = MinBaseIndex + BaseIndex;

	if (BlockIndex > MaxBaseIndex)
	{
		return 0;
	}

	TSize UpperPoolBlockSize = Pow2_64(BlockIndex);
	TSize LowerPoolBlockSize = BlockIndex == MinBaseIndex ? 0 : Pow2_64(BlockIndex - 1);

	TSize Spacing = (UpperPoolBlockSize - LowerPoolBlockSize) / SubIndexCount;
	TSize BlockSize = LowerPoolBlockSize + Spacing * (PoolIndex + 1);

	return BlockSize;
}

bool TMemPoolTable::GetBaseIndex(TSize BlockSize, TSize MinBaseIndex, TSize MaxBaseIndex, TSize& OutBaseIdx)
{
	TSize BlockIndex = CeilLog2(BlockSize);

	if (BlockIndex < MinBaseIndex)
	{
		OutBaseIdx = 0;
		return true;
	}

	TSize BaseIndex = BlockIndex - MinBaseIndex;

	if (BaseIndex > MaxBaseIndex - MinBaseIndex)
	{
		return false;
	}

	OutBaseIdx = BaseIndex;

	return true;
}

TSize TMemPoolTable::GetPoolIndex(TSize BlockSize, TSize BaseIndex, TSize MinBaseIndex, TSize SubIndexCountShift)
{
	TSize BlockIndex = MinBaseIndex + BaseIndex;
	//TSize UpperPoolBlockSize = Pow2_64(BlockIndex);
	TSize LowerPoolBlockSize = BlockIndex == MinBaseIndex ? 0 : ((TSize)1 << (BlockIndex - 1));

	TSize SpacingShift = BlockIndex == MinBaseIndex ? 5 : (BlockIndex - 1) - SubIndexCountShift;//(LowerPoolBlockSize) >> SubIndexCountShift;
	TSize PoolIndex = ((BlockSize - LowerPoolBlockSize) >> SpacingShift);

	if (PoolIndex == ((TSize)1 << SubIndexCountShift))
	{
		--PoolIndex;
	}

	return PoolIndex;
}

void TMemPool::Init(TSize BaseIndex, TSize PoolIndex, TSize BlockSize, TSize PoolBlockSize)
{
	this->PoolIndex = PoolIndex;
	this->BaseIndex = BaseIndex;
	this->BlockSize = BlockSize;
	this->PoolBlockSize = PoolBlockSize;
	this->PoolVMBlockSize = AlignToUpper(PoolBlockSize + MemPoolHdrSize, TVMBlock::GetPageSize());

#ifdef MALLOC_STATS
	Stats.BlockSize = BlockSize;
#endif
}

TMemPoolHdr* TMemPool::AddPool()
{
	if (BlockSize > PoolBlockSize)
	{
		PoolVMBlockSize = AlignToUpper(MemPoolHdrSize + MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize, TVMBlock::GetPageSize());
		
#ifdef MALLOC_SCALED_DEBUG
		printf("MALLOC: DBG: ADDING NEW MEM POOL; BIG BLOCK SIZE, ALLOCATED SIZE: %llu\n", PoolVMBlockSize);
#endif
	}
	else
	{
#ifdef MALLOC_SCALED_DEBUG
		printf("MALLOC: DBG: ADDING NEW MEM POOL; POOL BLOCK SIZE: %llu, ALLOCATED SIZE: %llu\n", PoolBlockSize, PoolVMBlockSize);
#endif
	}
	//TIMER
	//FUNC_TIME(bool Ok = NewPoolVMBlock.Allocate(PoolVMBlockSize));
	TVMBlock NewPoolVMBlock;
	//	printf("MALLOC: DBG: Last vm alloc time: %f ns\n", std::chrono::duration<float64, std::nano>(Ts.GetLastTime()).count());
	bool Ok = NewPoolVMBlock.Allocate(PoolVMBlockSize);
	//FUNC_TIME(bool Ok = NewPoolVMBlock.Allocate(PoolVMBlockSize));
	//printf("MALLOC: DBG: Last vm alloc time: %f ns\n", std::chrono::duration<float64, std::nano>(Ts.GetLastTime()).count());

	if (!Ok)
	{
#ifdef MALLOC_SCALED_DEBUG
		printf("MALLOC: DBG: VM BLOCK: Cannot allocate vm block\n");
#endif
		return nullptr;
	}

	PoolBlockSize = PoolVMBlockSize - MemPoolHdrSize;

	TMemPoolHdr* PoolHdr = (TMemPoolHdr*)NewPoolVMBlock.GetBase();
	new (PoolHdr) TMemPoolHdr{};
		
	PoolHdr->MemPool = this;
	PoolHdr->PoolVMBlock = move(NewPoolVMBlock);
	TSize BlockCount = PoolBlockSize / (MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize);
	PoolHdr->TotalBlockCount = BlockCount;
	PoolHdr->FreeBlockCount = BlockCount;

	++PoolCount;
	TotalFreeBlockCount += BlockCount;
	PoolList.PushBack(PoolHdr);
	HeadPool = PoolHdr;

#ifdef MALLOC_STATS
	Stats.AllocatedSize += PoolVMBlockSize;
	Stats.TotalBlockCount += BlockCount;
	Stats.FreeBlockCount += BlockCount;
#endif
	
	return PoolHdr;
}

void TMemPool::DeletePool(TMemPoolHdr* Pool)
{
	PoolList.Delete(Pool);
	--PoolCount;
	TotalFreeBlockCount -=Pool->FreeBlockCount;

#ifdef MALLOC_STATS
	Stats.AllocatedSize -= PoolVMBlockSize;
	Stats.TotalBlockCount -= Pool->TotalBlockCount;
	Stats.UsedBlockCount -= Pool->TotalBlockCount - Pool->FreeBlockCount;
	Stats.FreeBlockCount -= Pool->FreeBlockCount;
	Stats.Used -= Pool->Used;
#endif

	TVMBlock PoolVMBlock = move(Pool->PoolVMBlock); 
	PoolVMBlock.Free();
}

TMemPoolHdr* TMemPool::FindNewHeadPool()
{
	auto PoolNode = PoolList.GetFirst();

	TMemPoolHdr* Pool = nullptr;
	TMemPoolHdr* Head = nullptr;

	while (PoolNode)
	{
		Pool = *PoolNode->GetElement();

		if (HeadPool != Pool && Pool->FreeBlockCount >= (HeadPool->FreeBlockCount >> 1))
		{
			if (!Head)
			{
				Head = Pool;
			}
			else
			{
				if (Pool->FreeBlockCount > Head->FreeBlockCount)
				{
					Head = Pool;
				}
			}
		}

		PoolNode = PoolNode->GetNext();
	}

	return Head;
}


TMemBlockHdr* TMemPool::GetFreeBlock(TSize UsedSize)
{
	if (!HeadPool || HeadPool->FreeBlockCount == 0)
	{
			HeadPool = AddPool();

			if (!HeadPool)
			{
	//#ifdef MALLOC_SCALED_DEBUG
				printf("MALLOC: DBG: ERROR: OUT OF MEMORY: Cannot add memory pool\n");
	//#endif
				return nullptr;
			}
	}

	TMemBlockHdr* FreeBlock = nullptr;

	auto Pool = *HeadPool->GetElement();
	auto FoundBlock = Pool->FreeBlockList.PopBack();

	if (!FoundBlock)
	{
		if (Pool->ActiveBlocks != Pool->TotalBlockCount)
		{
			uint8* FirstBlock = (uint8*)(Pool + 1);
			auto InactiveBlock = (FirstBlock + (MemBlockHdrSize + MemBlockHdrOffsetSize + BlockSize) * Pool->ActiveBlocks);
			++Pool->ActiveBlocks;
			FreeBlock = (TMemBlockHdr*)InactiveBlock;
			new (FreeBlock) TMemBlockHdr{};

			FreeBlock->BlockSize = BlockSize;
			FreeBlock->PoolHdr = Pool;
		}
		else
		{
#ifdef MALLOC_SCALED_DEBUG
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
		--TotalFreeBlockCount;

#ifdef MALLOC_STATS
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
#ifdef MALLOC_STATS
			Pool->UsrBlockList.PushBack(FreeBlock);
#endif
		}
	
	return FreeBlock;
}

void TMemPool::Release()
{
	auto NextPoolNode = PoolList.GetFirst();

	while (NextPoolNode)
	{
		auto PoolToFree = *NextPoolNode->GetElement();
		NextPoolNode = NextPoolNode->GetNext();
		DeletePool(PoolToFree);
	}

	HeadPool = nullptr;

#ifdef MALLOC_STATS
	Stats = {};
#endif
}

void TMemPool::FreeUsrBlock(TMemBlockHdr* UsrBlock)
{
	if (UsrBlock)
	{
		auto Pool = UsrBlock->PoolHdr;
		TSize UsedSize = UsrBlock->UsedSize;

#ifdef MALLOC_STATS
		Pool->UsrBlockList.Delete(UsrBlock);
		Pool->Used -= UsrBlock->UsedSize;
#endif
		Pool->FreeBlockList.PushBack(UsrBlock);
		++Pool->FreeBlockCount;
		++TotalFreeBlockCount;

		if (Pool->FreeBlockCount == Pool->TotalBlockCount)
		{
			if (TotalFreeBlockCount > (Pool->FreeBlockCount + (Pool->FreeBlockCount >> 1)))
			{
				if (Pool == HeadPool)
				{
					auto NewHead = FindNewHeadPool();
					HeadPool = NewHead;
				}
				DeletePool(Pool);
				return;
			}
		}

		if (Pool->FreeBlockCount > HeadPool->FreeBlockCount)
		{
			HeadPool = Pool;
		}

#ifdef MALLOC_STATS
		Stats.Used -= UsrBlock->UsedSize;
		--Stats.UsedBlockCount;
#endif
		UsrBlock->UsedSize = 0;
	}
}

TSize TMemPool::GetBlockSize()
{
	return BlockSize;
}

TSize TMemPool::GetPoolCount()
{
	return PoolCount;
}

TMemPoolHdr* TMemPool::GetTop()
{
	return HeadPool;
}

TMemPool::TMemPoolStats* TMemPool::GetPoolStats()
{
#ifdef MALLOC_STATS
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

			else if (BlockHdr->UsedSize < Stats.BlockStats.MinUsedBlock)
			{
				Stats.BlockStats.MinUsedBlock = BlockHdr->UsedSize;
			}
		}
	}

	return &Stats;
#elif MALLOC_SCALED_DEBUG
	printf("MALLOC: DBG: Malloc stats are not enabled");
	return nullptr;
#else
	return nullptr;
#endif
}

bool TMemPoolTableEntry::Init(TSize BaseIndex, TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolBlockSize, TSize SubIndexCountShift)
{
	if (Initialized)
	{
		return false;
	}

	if (SubIndexCountShift > MaxSubIndexCountShift)
	{
		// log error;
		return false;
	}

	for (TSize i = 0; i < ((TSize)1 << SubIndexCountShift); ++i)
	{
		TSize BlockSize = TMemPoolTable::CalculatePoolBlockSize(BaseIndex, i, MinBaseBlockSize, MaxBaseBlockSize, ((TSize)1 << SubIndexCountShift)); 
		Pools[i].Init(BaseIndex, i, BlockSize, PoolBlockSize);
	}

	this->SubIndexCountShift = SubIndexCountShift;
	Initialized = true;
	return true;
}

void TMemPoolTableEntry::Release()
{
	for (TSize i = 0; i < ((TSize)1 << SubIndexCountShift); ++i)
	{
		Pools[i].Release();
	}

	Initialized = false;
}

TSize TMemPoolTableEntry::GetPoolCount()
{
	return ((TSize)1 << SubIndexCountShift);
}

TMemPool* TMemPoolTableEntry::GetPool(TSize PoolIndex)
{
	return &Pools[PoolIndex];
}

bool TMemPoolTable::TMemPoolTableStorage::Init(TSize EntryCount)
{
	bool Ok = false;
	TSize DataBlockSize = AlignToUpper(EntrySize * EntryCount, TVMBlock::GetPageSize());

	Ok = DataBlock.Allocate(DataBlockSize);

	if (!Ok)
	{
		return false;
	}

	FirstEntry = (TMemPoolTableEntry*)DataBlock.GetBase();
	LastEntry = FirstEntry + (EntryCount - 1);
	this->EntryCount = EntryCount;

	CreateElementsDefault(FirstEntry, EntryCount);

	return Ok;
}

TMemPoolTableEntry* TMemPoolTable::TMemPoolTableStorage::GetFirst()
{
	return FirstEntry;
}

TMemPoolTableEntry* TMemPoolTable::TMemPoolTableStorage::GetLast()
{
	return LastEntry;
}

TMemPoolTableEntry* TMemPoolTable::TMemPoolTableStorage::GetEntry(TSize Index)
{
	if (Index < EntryCount)
		return FirstEntry + Index;

	return nullptr;
}

TSize TMemPoolTable::TMemPoolTableStorage::GetEntryIndex(TMemPoolTableEntry* Entry)
{
	return Entry - FirstEntry;
}

TSize TMemPoolTable::TMemPoolTableStorage::GetEntryCount()
{
	return EntryCount;
}

void TMemPoolTable::TMemPoolTableStorage::Free()
{
	CreateElementsDefault(FirstEntry, EntryCount);
}

void TMemPoolTable::TMemPoolTableStorage::Release()
{
	DataBlock.Free();

	FirstEntry = nullptr;
	LastEntry = nullptr;
	EntryCount = 0;
	DataBlock = {};
}

TSize TMemPoolTable::GetPoolBlockSize()
{
	return PoolBlockSize;
}

TSize TMemPoolTable::GetEntryCount()
{
	return BaseEntries.GetEntryCount();
}

TSize TMemPoolTable::GetSubIndexCount()
{
	return SubIndexCount;
}

TSize TMemPoolTable::GetSubIndexCountShift()
{
	return SubIndexCountShift;
}

TSize TMemPoolTable::GetMinBaseBlockSize()
{
	return MinBaseBlockSize;
}

TSize TMemPoolTable::GetMaxBaseBlockSize()
{
	return MaxBaseBlockSize;
}

TSize TMemPoolTable::GetMinBaseIndex()
{
	return MinBaseIndex;
}

TSize TMemPoolTable::GetMaxBaseIndex()
{
	return MaxBaseIndex;
}

TMemPoolTableEntry* TMemPoolTable::GetEntry(TSize EntryNum)
{
	return &BaseEntries[EntryNum];
}

bool TMemPoolTable::Init(TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolBlockSize, TSize SubIndexCountShift)
{
#ifdef	MALLOC_SCALED_DEBUG
	if (!IsPow2(MinBaseBlockSize))
	{
		printf("MALLOC: DBG: ERROR: MinBaseBlocksize is not power of 2\n");
		return false;
	}

	if (!IsPow2(MaxBaseBlockSize))
	{
		printf("MALLOC: DBG: ERROR: MaxBaseBlocksize is not power of 2\n");
		return false;
	}

	if (PoolBlockSize % MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
	{
		printf("MALLOC: DBG: ERROR: PoolBlockSize is not aligned by MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT\n");
		return false;
	}

	//if (!IsPow2(SubIndexCount))
	//{
	//	printf("MALLOC: DBG: ERROR: SubIndexCount is not power of 2\n");
	//	return false;
	//}
#endif

	bool Ok = false;

	TSize EntryCount = CalculateNumOfBaseEntries(MinBaseBlockSize, MaxBaseBlockSize);

	Ok = BaseEntries.Init(EntryCount);

	if (Ok)
	{
		for (uint32 i = 0; i < EntryCount; ++i)
		{
			Ok = BaseEntries[i].Init(i, MinBaseBlockSize, MaxBaseBlockSize, PoolBlockSize, SubIndexCountShift);

			if (!Ok)
			{
				return false;
			}
		}

		this->PoolBlockSize    = PoolBlockSize;
		this->MaxBaseBlockSize = MaxBaseBlockSize;
		this->MaxBaseIndex     = Log2_64(MaxBaseBlockSize);
		this->MinBaseBlockSize = MinBaseBlockSize;
		this->MinBaseIndex     = Log2_64(MinBaseBlockSize);
		this->SubIndexCount    = (TSize)1 << SubIndexCountShift;
		this->SubIndexCountShift = SubIndexCountShift;

		return true;
	}

	return false;
}

void TMemPoolTable::Release()
{
	TSize EntryCount = BaseEntries.GetEntryCount();

	for (uint32 i = 0; i < EntryCount; ++i)
	{
		BaseEntries[i].Release();
	}

	BaseEntries.Release();
}

void TMemPoolTable::UpdateStats()
{
#ifdef MALLOC_STATS
	TSize EntryCount = BaseEntries.GetEntryCount();

	for (TSize i = 0; i < EntryCount; ++i)
	{
		TSize PoolCount = BaseEntries[i].GetPoolCount();
		for (TSize j = 0; j < PoolCount; ++j)
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

#elif MALLOC_SCALED_DEBUG
	printf("MALLOC: DBG: Malloc stats are not enabled");
#endif
}



void* TMallocScaled::MallocInternal(TSize Size, TSize Alignment)
{
#ifdef MALLOC_SCALED_DEBUG
	printf("\nMALLOC: DBG: ALLOCATE MEM BLOCK: Size: %llu, Alignment: %llu\n", Size, Alignment);
#endif

#ifdef MALLOC_STATS
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

#ifdef MALLOC_TIME_STATS
	TTimer Timer;
	Timer.Start();
#endif

	void* UsrBlockPtr = nullptr;

	if (!Alignment)
	{
		Alignment = MALLOC_SCALED_DEFAULT_ALIGNMENT;
	}

	//if (Size && IsPow2(Alignment) && Alignment <= TVMBlock::GetPageSize())
	if (Size)
	{
		TSize AdjustedBlockSize = Size + (Size >> MALLOC_SCALED_ALLOCATION_ADJUSTMENT);
		AdjustedBlockSize = AlignToUpper(AdjustedBlockSize + Alignment, MALLOC_SCALED_DEFAULT_ALIGNMENT);

		TSize BaseIndex = 0;
		bool Ok = TMemPoolTable::GetBaseIndex(AdjustedBlockSize, PoolTable.GetMinBaseIndex(), PoolTable.GetMaxBaseIndex(), BaseIndex);

		if (Ok)
		{
			TSize PoolIdx = TMemPoolTable::GetPoolIndex(AdjustedBlockSize, BaseIndex, PoolTable.GetMinBaseIndex(), PoolTable.GetSubIndexCountShift());
			TMemPoolTableEntry* BaseEntry = PoolTable.GetEntry(BaseIndex);

			TMemPool* Pool = BaseEntry->GetPool(PoolIdx);

			//TIMER
			//FUNC_TIME(TMemBlockHdr * FreeBlock = Pool->GetFreeBlock(Size));
			//printf("MALLOC: DBG: Last find free block time: %f ns\n", std::chrono::duration<float64, std::nano>(Ts.GetLastTime()).count());

			TMemBlockHdr* FreeBlock = Pool->GetFreeBlock(Size);
	

			if (FreeBlock)
			{
				UsrBlockPtr = FreeBlock + 1;
				void* AlignedPtr = AlignToUpper(UsrBlockPtr, Alignment);
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

#ifdef MALLOC_TIME_STATS
	Timer.Stop();
	MallocTimeStats.BlockAllocTime += Timer.GetDuration();
	std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
	printf("MALLOC: DBG: Allocation block Size: %llu, Alignment: %llu at address: 0x%p Time: %f ms\n", Size, Alignment, UsrBlockPtr, d.count());
#endif

#ifdef MALLOC_SCALED_DEBUG
	if (UsrBlockPtr)
	{
		printf("MALLOC: DBG: ALLOCATE MEM BLOCK -[OK]: Size: %llu, Alignment: %llu at address %p\n", Size, Alignment, UsrBlockPtr);
	}
	else
	{
		printf("MALLOC: DBG: ALLOCATE MEM BLOCK -[FAILED]: Size: %llu, Alignment: %llu at address %p - \n", Size, Alignment, UsrBlockPtr);
	}	
#endif

#ifdef MALLOC_STATS
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

void* TMallocScaled::ReallocInternal(void* Addr, TSize NewSize, TSize NewAlignment)
{
	TSize OldSize = 0; 
#ifdef MALLOC_SCALED_DEBUG
	OldSize = GetSizeInternal(Addr);
	printf("\nMALLOC: DBG: REALLOCATE MEM BLOCK: Address: 0x%p, Old size: %llu, New size: %llu, New alignment: %llu\n", Addr, OldSize, NewSize, NewAlignment);
#endif

#ifdef MALLOC_STATS
	OldSize = GetSizeInternal(Addr);
	++MallocStats.RequestStats.ReallocRequests;
#endif

#ifdef MALLOC_TIME_STATS
	OldSize = GetSizeInternal(Addr);
	TTimer Timer;
	Timer.Start();
#endif

	void* UsrBlockPtr = nullptr;

	if (!NewAlignment)
	{
		NewAlignment = MALLOC_SCALED_DEFAULT_ALIGNMENT;
	}

	if (NewSize && Addr && IsPow2(NewAlignment) && NewAlignment <= TVMBlock::GetPageSize())
	{
		void* NewPtr = nullptr;

		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;

		TSize AdjustedNewSize = NewSize + (NewSize >> MALLOC_SCALED_ALLOCATION_ADJUSTMENT);
		AdjustedNewSize = AlignToUpper(AdjustedNewSize + NewAlignment, MALLOC_SCALED_DEFAULT_ALIGNMENT);

		if (AdjustedNewSize > Block->BlockSize || AdjustedNewSize < (Block->BlockSize >> MALLOC_SCALED_REALLOCATION_ADJUSTMENT))
		{
			NewPtr = MallocInternal(NewSize, NewAlignment);
		}
		else
		{
			OldSize = Block->UsedSize;
			Block->UsedSize = NewSize;
			
			void* AlignedPtr = AlignToUpper(Addr, NewAlignment);
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
#ifdef MALLOC_STATS
			++MallocStats.RequestStats.FailedReallocInPlaceRequests;
#endif
			TSize SizeToCopy = NewSize < OldSize ? NewSize : OldSize;
			memcpy(NewPtr, Addr, SizeToCopy);
			FreeInternal(Addr);
			UsrBlockPtr = NewPtr;
		}

#ifdef MALLOC_TIME_STATS
		Timer.Stop();
		MallocTimeStats.BlockReallocTime += Timer.GetDuration();
		std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
		printf("MALLOC: DBG: Reallocation block Old size: %llu Address: 0x%p; New size: %llu New address: 0x%p, New alignment: %llu, Time : %f ms\n", OldSize, Addr, NewSize, UsrBlockPtr, NewAlignment, d.count());
#endif

#ifdef MALLOC_SCALED_DEBUG
		if (UsrBlockPtr)
		{
			printf("MALLOC: DBG: REALLOCATE MEM BLOCK -[OK]: Old size: %llu at address %p; New size %llu at new address %p, new alignment: %llu - \n", OldSize, Addr, NewSize, UsrBlockPtr, NewAlignment);
		}
		else
		{
			printf("MALLOC: DBG: REALLOCATE MEM BLOCK -[FAILED]: Old size: %llu at address %p, new alignment: %llu - \n", NewSize, Addr, NewAlignment);
		}
#endif

#ifdef MALLOC_STATS
		if (UsrBlockPtr)
		{
			if (!NewPtr)
			{
				MallocStats.TotalStats.TotalUsed -= OldSize;
				MallocStats.TotalStats.TotalUsed += NewSize;

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

	return UsrBlockPtr;
}

void TMallocScaled::FreeInternal(void* Addr)
{
#ifdef MALLOC_SCALED_DEBUG
	TSize Size0 = GetSizeInternal(Addr);
	printf("\nMALLOC: DBG: FREE MEM BLOCK: Address: 0x%p, Size: %llu\n", Addr, Size0);
#endif

#ifdef MALLOC_STATS
	TSize Size1 = GetSizeInternal(Addr);
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

#ifdef MALLOC_TIME_STATS
	TTimer Timer;
	TSize Size2 = GetSizeInternal(Addr);
	Timer.Start();
#endif

	bool Ok = true;
	if (Addr)
	{
		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;
		TMemPoolHdr* PoolHdr = Block->PoolHdr;
		PoolHdr->MemPool->FreeUsrBlock(Block);	

#ifdef MALLOC_TIME_STATS
		Timer.Stop();
		MallocTimeStats.BlockFreeTime += Timer.GetDuration();
		std::chrono::duration<float64, std::milli> d = Timer.GetDuration();
		printf("MALLOC: DBG: Free block Address: 0x%p, Size: %llu Time : %f ms\n", Addr, Size2, d.count());
#endif

#ifdef MALLOC_SCALED_DEBUG
		if (Ok)
		{
			printf("MALLOC: DBG: FREE MEM BLOCK -[OK]: Address: 0x%p, Size: %llu\n", Addr, Size0);
		}
		else
		{
			printf("MALLOC: DBG: FREE MEM BLOCK -[FAILED]: Address: 0x%p, Size: %llu\n", Addr, Size0);
		}
#endif

#ifdef MALLOC_STATS
		if (Ok)
		{
			MallocStats.TotalStats.TotalUsed -= Size1;
		}
		else
		{
			++MallocStats.RequestStats.FailedFreeRequests;
		}
#endif
	}

}

TSize TMallocScaled::GetSizeInternal(void* Addr)
{
	TSize UsedSize = 0;

	if (Addr)
	{
		TMemBlockHdrOffset* Offset = (TMemBlockHdrOffset*)(Addr);
		--Offset;
		TMemBlockHdr* Block = Offset->BlockHdr;
		UsedSize = Block->UsedSize;
	}

	return UsedSize;
}

void* TMallocScaled::Malloc(TSize Size, TSize Alignment)
{
	Guard.Lock();
	void* FreeBlock = MallocInternal(Size, Alignment);
	Guard.Unlock();
	return FreeBlock;
}

void* TMallocScaled::Realloc(void* Addr, TSize Size, TSize Alignment)
{
	Guard.Lock();
	void* ReallocatedBlock = ReallocInternal(Addr, Size, Alignment);
	Guard.Unlock();
	return ReallocatedBlock;
}

void  TMallocScaled::Free(void* Addr)
{
	Guard.Lock();
	FreeInternal(Addr);
	Guard.Unlock();
}

TSize TMallocScaled::GetSize(void* Addr)
{
	Guard.Lock();
	TSize UsedSize = GetSizeInternal(Addr);
	Guard.Unlock();
	return UsedSize;
}

void TMallocScaled::DebugInit(TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolBlockSize, uint32 SubIndexCount)
{
#ifdef MALLOC_SCALED_DEBUG
	bool Ok = false;

	if (!Initialized)
	{
		Ok = TVMBlock::Init();
		printf("MALLOC: DBG: DEBUG initialization of memory allocator...\n");
		if (Ok)
		{
			Ok = PoolTable.Init(MinBaseBlockSize,
				MaxBaseBlockSize,
				PoolBlockSize,
				SubIndexCount);

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

bool TMallocScaled::Init()
{
	bool Ok = false;

	if (!Initialized)
	{
		Ok = TVMBlock::Init();
#ifdef MALLOC_SCALED_DEBUG
		printf("MALLOC: DBG: Initializing memory allocator...\n");
#endif
		if (Ok)
		{
			Ok = PoolTable.Init(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE,
				MALLOC_SCALED_MAX_BASE_BLOCK_SIZE,
				MALLOC_SCALED_POOL_BLOCK_SIZE,
				Log2_64(MALLOC_SCALED_SUBINDEX_COUNT));
		
			if (Ok)
			{
				Initialized = true;
				return true;
			}
		}
	}
	else
	{
#ifdef MALLOC_SCALED_DEBUG
		printf("MALLOC: DBG: Memory allocator is initialized already...\n");
#endif
	}

	return false;
}

bool TMallocScaled::IsInitialized()
{
	return Initialized;
}


void TMallocScaled::Shutdown()
{
#ifdef MALLOC_SCALED_DEBUG
	printf("MALLOC: DBG: Destroying memory allocator\n");
#endif

	PoolTable.Release();
	TVMBlock::Release();

#ifdef MALLOC_STATS
	MallocStats = {};
	MallocTimeStats = {};
#endif

	Initialized = false;

#ifdef MALLOC_SCALED_DEBUG
	printf("MALLOC: DBG: Destroying memory allocator - Ok\n");
#endif
}

TSize TMallocScaled::GetMallocMaxAlignment()
{
	return TVMBlock::GetPageSize();
}

void TMallocScaled::GetSpecificStats(void* OutStatData)
{
//#ifdef MALLOC_STATS
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
//#ifdef MALLOC_SCALED_DEBUG
//	printf("MALLOC: DBG: Malloc stats are not enabled");
//#endif
//#endif
}

TSize TMallocScaled::GetBaseEntryCount()
{
	return PoolTable.GetEntryCount();
}

TSize TMallocScaled::GetBlockSize(TSize BaseIndex, TSize PoolIndex)
{
	TMemPoolTableEntry* BaseEntry = PoolTable.GetEntry(BaseIndex);

	TMemPool* Pool = BaseEntry->GetPool(PoolIndex);

	return Pool->GetBlockSize();
}

TSize TMallocScaled::GetBlockCount(TSize BaseIndex, TSize PoolIndex)
{
	TMemPoolTableEntry* BaseEntry = PoolTable.GetEntry(BaseIndex);
	TMemPool* Pool = BaseEntry->GetPool(PoolIndex);
	TSize BlockCount = PoolTable.GetPoolBlockSize() / (Pool->GetBlockSize() + MemBlockHdrSize + MemBlockHdrOffsetSize);

	return BlockCount;
}

TSize TMallocScaled::GetMaxPoolBlockSize()
{
	return PoolTable.GetPoolBlockSize();
}

float64 GetFunctionTime()
{
	return std::chrono::duration<float64, std::nano>(Ts.GetAvgTime()).count();
}