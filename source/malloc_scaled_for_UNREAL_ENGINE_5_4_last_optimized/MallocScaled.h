// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "MallocScaledVMBlock.h"
#include "Templates/AlignmentTemplates.h"
#include "HAL/CriticalSection.h"
#include "HAL/MemoryBase.h"
#include "Math/UnrealMathUtility.h"


#include <chrono>

#define USE_SCALED_MALLOC 1

static const SIZE_T MALLOC_SCALED_DEFAULT_ALIGNMENT = 16;
static const SIZE_T MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT = 16;
static const SIZE_T MALLOC_SCALED_SUBINDEX_COUNT = 8;
static const SIZE_T MALLOC_SCALED_MAX_SUBINDEX_COUNT = 16;
static const SIZE_T MALLOC_SCALED_MIN_BASE_BLOCK_SIZE = 256;        // Bytes;
static const SIZE_T MALLOC_SCALED_MAX_BASE_BLOCK_SIZE = 17179869184; // Bytes;
static const SIZE_T MALLOC_SCALED_POOL_BLOCK_SIZE = 16777216;    // Previous val: 524288 Bytes;

#define MALLOC_SCALED_MALLOC_STATS 0
#define MALLOC_SCALED_MALLOC_TIME_STATS 0
#define MALLOC_SCALED_MALLOC_DEBUG 0

static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_DEFAULT_ALIGNMENT), "MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT must be power of 2");
static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "DEFAULT_ALIGNMENT must be power of 2");
static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_MAX_SUBINDEX_COUNT), "MAX_SUBINDEX_COUNT must be power of 2");
static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_SUBINDEX_COUNT), "SUBINDEX_COUNT must be power of 2");
static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE), "MIN_BASE_BLOCK_SIZE must be power of 2");
static_assert(FMath::IsPowerOfTwo(MALLOC_SCALED_MAX_BASE_BLOCK_SIZE), "MAX_BASE_BLOCK_SIZE must be power of 2");
static_assert(IsAligned(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE / MALLOC_SCALED_SUBINDEX_COUNT, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "the smallest block size must be aligned by DEFAULT_ALIGNMENT");
static_assert(IsAligned(MALLOC_SCALED_POOL_BLOCK_SIZE, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "commited pool size must be aligned by DEFAULT_ALIGNMENT");

typedef std::chrono::duration<std::chrono::high_resolution_clock::rep, std::nano> TMallocScaledDuration;

struct TMallocScaledBlockStats
{
	TMallocScaledBlockStats() :
		MaxUsedBlock(0),
		MinUsedBlock(std::numeric_limits<SIZE_T>::max()),
		LargestUsedBlock(0),
		SmallestUsedBlock(std::numeric_limits<SIZE_T>::max())

	{

	}

	SIZE_T MaxUsedBlock;
	SIZE_T MinUsedBlock;
	SIZE_T LargestUsedBlock;
	SIZE_T SmallestUsedBlock;
};

struct TMallocScaledStats
{
	struct TTotalStats
	{
		TTotalStats() :
			TotalUsed(0),
			TotalAllocated(0),
			TotalReserved(0),
			MaxTotalUsed(0),
			MaxTotalAllocated(0)
		{
		}

		SIZE_T TotalUsed; //~ 
		SIZE_T TotalAllocated;//~
		SIZE_T TotalReserved;

		/*
		*	Max peaks of memory using by allocator from all time since its startup
		*/

		SIZE_T MaxTotalUsed;//+
		SIZE_T MaxTotalAllocated;//+

	} TotalStats;

	struct TRequestStats
	{
		TRequestStats() :
			MallocRequests(0),
			ReallocRequests(0),
			FreeRequests(0),
			FailedMallocRequests(0),
			FailedReallocRequests(0),
			FailedReallocInPlaceRequests(0),
			FailedFreeRequests(0)

		{
		}

		uint32 MallocRequests;
		uint32 ReallocRequests;
		uint32 FreeRequests;

		uint32 FailedMallocRequests;
		uint32 FailedReallocRequests;
		uint32 FailedReallocInPlaceRequests;
		uint32 FailedFreeRequests;

	} RequestStats;

	struct TRequestVals
	{
		TRequestVals() :
			MaxAllocRequestSize(0),
			MinAllocRequestSize(std::numeric_limits<SIZE_T>::max()),
			MaxFreeRequestSize(0),
			MinFreeRequestSize(std::numeric_limits<SIZE_T>::max())
		{

		}

		SIZE_T MaxAllocRequestSize;
		SIZE_T MinAllocRequestSize;

		SIZE_T MaxFreeRequestSize;
		SIZE_T MinFreeRequestSize;

	} RequestVals;

	TMallocScaledBlockStats BlockStats;
};

class TMallocScaledTimeStats
{
public:
	TMallocScaledTimeStats() :
		MaxTime(0),
		MinTime(TMallocScaledDuration::max()),
		AvgTime(0),
		LastTime(0),
		TotalTime(0),
		MeasureCount(0)
	{

	}

	void Reset();
	void AddInterval(TMallocScaledDuration TimeInterval);

	TMallocScaledTimeStats& operator+(TMallocScaledDuration TimeInterval);
	TMallocScaledTimeStats& operator+=(TMallocScaledDuration TimeInterval);

	TMallocScaledDuration GetMaxTime();
	TMallocScaledDuration GetMinTime();
	TMallocScaledDuration GetAvgTime();
	TMallocScaledDuration GetLastTime();
	TMallocScaledDuration GetTotalTime();

private:
	TMallocScaledDuration MaxTime;
	TMallocScaledDuration MinTime;
	TMallocScaledDuration AvgTime;
	TMallocScaledDuration LastTime;
	TMallocScaledDuration TotalTime;

	uint64 MeasureCount;
};

struct TMallocTimeStatsCommon
{
	TMallocScaledTimeStats BlockAllocTime;
	TMallocScaledTimeStats BlockReallocTime;
	TMallocScaledTimeStats BlockFreeTime;
};

class FMallocScaledImpl
{
	template<typename TELEM>
	class TListNode
	{
	public:
		TListNode() :
			Next(nullptr),
			Prev(nullptr),
			Element{}
		{

		}

		TListNode(const TELEM& Elem) :
			Element(Elem),
			Next(nullptr),
			Prev(nullptr)
		{
		}

		TListNode(TELEM&& Elem) :
			Element(std::move(Elem)),
			Next(nullptr),
			Prev(nullptr)
		{
		}

		TListNode& operator=(const TListNode& Src)
		{
			if (&Src != this)
			{
				Element = Src.Element;
			}

			return *this;
		}

		TListNode& operator=(TListNode&& Src)
		{
			if (&Src != this)
			{
				Element = move(Src.Element);
			}

			return *this;
		}

		void LinkAfter(TListNode* Node)
		{
			if (Node)
			{
				Next = Node->Next;
				Prev = Node;

				if (Node->Next)
					Node->Next->Prev = this;

				Node->Next = this;
			}
		}

		void LinkBefore(TListNode* Node)
		{
			if (Node)
			{
				Next = Node;
				Prev = Node->Prev;

				if (Node->Prev)
					Node->Prev->Next = this;

				Node->Prev = this;
			}
		}

		void Unlink()
		{
			if (Prev)
				Prev->Next = Next;

			if (Next)
				Next->Prev = Prev;

			Next = nullptr;
			Prev = nullptr;
		}

		void LinkReplace(TListNode* Node)
		{
			if (Node)
			{
				Next = Node->Next;
				Prev = Node->Prev;

				Next = nullptr;
				Prev = nullptr;
			}
		}

		void Reverse()
		{
			swap(Next, Prev);
		}

		bool IsFirst() const
		{
			return (Next && !Prev);
		}

		bool IsLast() const
		{
			return (!Next && Prev);
		}

		bool IsSingle() const
		{
			return (!Next && !Prev);
		}

		bool IsAdjecent(const TListNode* Node) const
		{
			return (Node &&
				(Next == Node && Node->Prev == this ||
					Prev == Node && Node->Next == this));
		}

		TListNode* GetNext() const
		{
			return Next;
		}

		TListNode* GetPrev() const
		{
			return Prev;
		}

		void SetNext(TListNode* Node)
		{
			Next = Node;
		}

		void SetPrev(TListNode* Node)
		{
			Prev = Node;
		}

		auto* GetElement()
		{
			return &Element;
		}

		const auto* GetElement() const
		{
			return &Element;
		}

		auto* operator->()
		{
			return &Element;
		}

		const auto* operator->() const
		{
			return &Element;
		}

		auto& operator*()
		{
			return Element;
		}

		const auto& operator*() const
		{
			return Element;
		}

		~TListNode()
		{

		}

	private:
		TELEM Element;
		TListNode* Next;
		TListNode* Prev;
	};

	template<typename TNODE>
	class TListBase
	{
	public:
		using TNode = TNODE; // FList::TListNode<TELEM>;

		class TIterator
		{
		public:

			TIterator() :
				Node(nullptr),
				First(nullptr),
				Last(nullptr)
			{
			}

			TIterator(TNode* First, TNode* Last, TNode* Node) :
				Node(Node),
				First(First),
				Last(Last)
			{
			}

			TIterator(const TIterator& Src) :
				Node(Src.Node),
				First(Src.First),
				Last(Src.Last)
			{
			}

			TIterator(TIterator&& Src) :
				Node(Src.Node),
				First(Src.First),
				Last(Src.Last)
			{
				Src = nullptr;
			}

			TIterator(nullptr_t) :
				Node(nullptr),
				First(nullptr),
				Last(nullptr)
			{

			}

			TIterator& operator=(const TIterator& Src)
			{
				if (&Src != this)
				{
					Node = Src.Node;
					First = Src.First;
					Last = Src.Last;
				}
				return *this;
			}

			TIterator& operator=(TIterator&& Src)
			{
				if (&Src != this)
				{
					Node = Src.Node;
					First = Src.First;
					Last = Src.Last;
					Src = nullptr;
				}
				return *this;
			}

			TIterator& operator=(nullptr_t Src)
			{
				Node = nullptr;
				First = nullptr;
				Last = nullptr;
				return *this;
			}

			TIterator operator++()
			{
				Node = Node->GetNext();
				return *this;
			}

			TIterator operator++(int)
			{
				TIterator Copy = *this;
				++*this;
				return Copy;
			}

			TIterator operator+(uint32_t Distance)
			{
				auto Tmp = *this;
				while (*this && Distance)
				{
					++*this;
					--Distance;
				}
				return Tmp;
			}

			TIterator operator-(uint32_t Distance)
			{
				auto Tmp = *this;
				while (*this && Distance)
				{
					--*this;
					--Distance;
				}
				return Tmp;
			}

			TIterator operator--()
			{
				Node = Node->GetPrev();
				return *this;
			}

			TIterator operator--(int)
			{
				TIterator Copy = *this;
				--*this;
				return Copy;
			}

			void Reset()
			{
				Node = First;
			}

			TNode* GetNode()
			{
				return Node;
			}

			auto& operator*()
			{
				return **Node;
			}

			const auto& operator*() const
			{
				return **Node;
			}

			auto* operator->()
			{
				return &(**Node);
			}

			const auto* operator->() const
			{
				return &(**Node);
			}

			bool operator==(const TIterator& IterB)
			{
				return (Node == IterB.Node);
			}

			bool operator!=(const TIterator& IterB)
			{
				return !(*this == IterB);
			}

			explicit operator bool() const
			{
				return Node != nullptr;
			}

		private:
			TNode* Node;
			TNode* First;
			TNode* Last;
		};

		using TIter = TIterator;

		TListBase() :
			FirstNode(nullptr),
			LastNode(nullptr)
		{

		}

		TListBase(TNode* First, TNode* Last) :
			FirstNode(First),
			LastNode(Last)
		{

		}

		void PushBack(TNode* Node)
		{
			InsertAfter(LastNode, Node);
		}

		TNode* PopBack()
		{
			TNode* Last = LastNode;
			Delete(Last);

			return Last;
		}

		void PushFront(TNode* Node)
		{
			InsertBefore(FirstNode, Node);
		}

		TNode* PopFront()
		{
			TNode* Front = FirstNode;
			Delete(Front);

			return Front;
		}

		void SwapLists(TListBase& Src)
		{
			if (&Src != this)
			{
				swap(FirstNode, Src.FirstNode);
				swap(LastNode, Src.LastNode);
			}
		}

		void Reverse()
		{
			TNode* Next = nullptr;
			TNode* Pos = FirstNode;

			while (Pos)
			{
				Next = Pos->GetNext();
				Pos->Reverse();
				Pos = Next;
			}

			swap(FirstNode, LastNode);
		}

		bool IsEmpty() const
		{
			return !FirstNode;
		}

		void InsertListAfter(TNode* Pos, TNode* First, TNode* Last)
		{
			if (!FirstNode)
			{
				FirstNode = First;
				LastNode = Last;
			}
			else
			{
				if (First && Last)
				{
					TNode* PosNext = Pos->GetNext();

					First->SetPrev(Pos);
					Last->SetNext(PosNext);

					if (PosNext)
						Pos->GetNext()->SetPrev(Last);

					Pos->SetNext(First);
				}
			}
		}

		void InsertListBefore(TNode* Pos, TNode* First, TNode* Last)
		{
			if (!FirstNode)
			{
				FirstNode = First;
				LastNode = Last;
			}
			else
			{
				if (First && Last)
				{
					TNode* PosPrev = Pos->GetPrev();

					Last->SetNext(Pos);
					First->SetPrev(PosPrev);

					if (PosPrev)
						Pos->GetPrev()->SetNext(First);

					Pos->SetPrev(Last);
				}
			}
		}

		void InsertAfter(TNode* Pos, TNode* Node)
		{
			if (!FirstNode)
			{
				FirstNode = Node;
				LastNode = Node;
			}
			else
			{
				if (Node)
					Node->LinkAfter(Pos);

				if (Pos == LastNode)
					LastNode = Node;
			}
		}

		void InsertBefore(TNode* Pos, TNode* Node)
		{
			if (!FirstNode)
			{
				FirstNode = Node;
				LastNode = Node;
			}
			else
			{
				if (Node)
					Node->LinkBefore(Pos);

				if (Pos == FirstNode)
					FirstNode = Node;
			}
		}

		// relocate Elem to after Pos;
		void RelocateAfter(TIterator Pos, TIterator Elem)
		{
			TNode* PosNode = Pos.GetNode();
			TNode* ElemNode = Elem.GetNode();
			Delete(ElemNode);
			InsertAfter(PosNode, ElemNode);
		}

		void RelocateBefore(TIterator Pos, TIterator Elem)
		{
			TNode* PosNode = Pos.GetNode();
			TNode* ElemNode = Elem.GetNode();
			Delete(ElemNode);
			InsertBefore(PosNode, ElemNode);
		}

		void Delete(TNode* Node)
		{
			if (Node)
			{
				TNode* Next = Node->GetNext();
				TNode* Prev = Node->GetPrev();

				if (FirstNode == Node)
					FirstNode = Next;

				if (LastNode == Node)
					LastNode = Prev;

				Node->Unlink();
			}
		}

		void Delete()
		{
			FirstNode = nullptr;
			LastNode = nullptr;
		}

		TNode* GetLastNode(TNode* First, uint32_t Count)
		{
			if (First)
			{
				while (Count)
				{
					First = First->GetNext();
					--Count;
				}
			}

			return First;
		}

		TNode* GetFirst() const
		{
			return FirstNode;
		}

		TNode* GetLast() const
		{
			return LastNode;
		}

		TNode* GetFirst()
		{
			return FirstNode;
		}

		TNode* GetLast()
		{
			return LastNode;
		}

		TIter GetFirstIt()
		{
			return TIter(FirstNode, LastNode, FirstNode);
		}

		TIter GetLastIt()
		{
			return TIter(FirstNode, LastNode, LastNode);
		}


		TIter GetBegin()
		{
			return TIter(FirstNode, LastNode, FirstNode);
		}

		TIter GetEnd()
		{
			if (LastNode)
				return TIter(FirstNode, LastNode, LastNode->GetNext());
			return nullptr;
		}

		void SetFirst(TNode* First)
		{
			FirstNode = First;
		}

		void SetLast(TNode* Last)
		{
			LastNode = Last;
		}

	private:
		TNode* FirstNode;
		TNode* LastNode;
	};

public:
	class TMemPool;
	struct TMemPoolHdr;
	struct TMemBlockHdr;

	struct alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
		TMemBlockHdrOffset
	{
		TMemBlockHdrOffset()
		{
			BlockHdr = nullptr;
		}

		TMemBlockHdr* BlockHdr;
	};

	using TMemBlockHdrBase = TListNode<TMemBlockHdr*>;

	struct alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
		TMemBlockHdr : public TMemBlockHdrBase
	{
		TMemBlockHdr() :
			TMemBlockHdrBase(this)
		{
			UsedSize = 0;
			BlockSize = 0;
			PoolHdr = nullptr;
		}

		friend bool operator<(TMemBlockHdr& BlockA, TMemBlockHdr& BlockB)
		{
			return BlockA.UsedSize < BlockB.UsedSize;
		}

		friend bool operator>(TMemBlockHdr& BlockA, TMemBlockHdr& BlockB)
		{
			return BlockA.UsedSize > BlockB.UsedSize;
		}

		SIZE_T UsedSize;  // !!! UsedSize = BlockSize - RestFreeSize;
		SIZE_T BlockSize; // !!! Size of user free space without block header;
		TMemPoolHdr* PoolHdr;
	};

	//	Memory block structure;
//	____________
//	|	HDR	   |
//	|----------| <--*
//	|	USED   |    |
//	|----------| (BLOCK SIZE)
//	|   FREE   |    |
//	|__________| <--*

	using TMemPoolHdrBase = TListNode<TMemPoolHdr*>;

	struct alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
		TMemPoolHdr :
		public TMemPoolHdrBase
	{
		using TMemBlockList = TListBase<TMemBlockHdrBase>;

		TMemPoolHdr() :
			TMemPoolHdrBase(this)
		{
#if MALLOC_SCALED_MALLOC_STATS
			Used = 0;
#endif
			ActiveBlocks = 0;
			TotalBlockCount = 0;
			FreeBlockCount = 0;

			MemPool = nullptr;
		}
#if MALLOC_SCALED_MALLOC_STATS
		SIZE_T Used;
#endif
		SIZE_T ActivePools;
		SIZE_T ReservedPoolCount;
		SIZE_T TotalPoolCount;

		SIZE_T ActiveBlocks;
		SIZE_T FreeBlockCount;
		SIZE_T TotalBlockCount;

		TMemPool* MemPool;
		TMemBlockList FreeBlockList;

#if MALLOC_SCALED_MALLOC_STATS
		TMemBlockList UsrBlockList;
#endif
		FMallocScaledVMBlock PoolVMBlock;
	};

	using TMemPoolList = TListBase<TMemPoolHdrBase>;

	static const SIZE_T MemBlockHdrOffsetSize = sizeof(TMemBlockHdrOffset);
	static const SIZE_T MemBlockHdrSize = sizeof(TMemBlockHdr);
	static const SIZE_T MemPoolHdrSize = sizeof(TMemPoolHdr);

	static_assert(IsAligned(MemPoolHdrSize, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "Memory pool header size must be aligned by DEFAULT_ALIGNMENT");
	static_assert(IsAligned(MemBlockHdrSize, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "Memory block header size must be aligned by DEFAULT_ALIGNMENT");

	class alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
		TMemPool
	{
	public:

		struct TMemPoolStats
		{
			TMemPoolStats()
			{
				Used = 0;
				PeakUsed = 0;

				UsedBlockCount = 0;
				PeakUsedBlockCount = 0;
				FreeBlockCount = 0;
				TotalBlockCount = 0;

				BlockSize = 0;
				AllocatedSize = 0;
			}

			SIZE_T Used;
			SIZE_T PeakUsed;

			SIZE_T UsedBlockCount;
			SIZE_T PeakUsedBlockCount;
			SIZE_T FreeBlockCount;
			SIZE_T TotalBlockCount;

			SIZE_T BlockSize;
			TMallocScaledBlockStats BlockStats;

			SIZE_T AllocatedSize;
		};

		TMemPool()
		{
			PoolIndex = 0;
			BaseIndex = 0;
			PoolCount = 0;
			TotalFreeBlockCount = 0;
			HeadPool = nullptr;
			BlockSize = 0;

			PoolBlockSize = 0;
			PoolVMBlockSize = 0;

		}

		void Init(SIZE_T InBaseIndex, SIZE_T InPoolIndex, SIZE_T InBlockSize, SIZE_T InPoolBlockSize);
		void Release();

		TMemBlockHdr* GetFreeBlock(SIZE_T AdjustedUsedSize, SIZE_T UsedSize);

		void FreeUsrBlock(TMemBlockHdr* UsrBlock);

		SIZE_T GetBlockSize();
		SIZE_T GetPoolCount();

		TMemPoolStats* GetPoolStats();
	private:
		TMemPoolHdr* FindFreePool();
		TMemPoolHdr* FindNewHeadPool();

		TMemPoolHdr* AddPool();
		void DeletePool(TMemPoolHdr*);

		SIZE_T PoolIndex;
		SIZE_T BaseIndex;
		SIZE_T PoolCount;
		SIZE_T TotalFreeBlockCount;

		TMemPoolHdr* HeadPool;

		SIZE_T  BlockSize;

		SIZE_T PoolBlockSize;
		SIZE_T PoolVMBlockSize;

		TMemPoolList PoolList;

#if MALLOC_SCALED_MALLOC_STATS
		TMemPoolStats Stats;
#endif
	};

	class alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT)
		TMemPoolTableEntry
	{
	public:
		TMemPoolTableEntry()
		{
			Initialized = false;
			SubIndexCountShift = 0;
			MaxSubIndexCountShift = FMallocScaledMath::Log2_64(MALLOC_SCALED_MAX_SUBINDEX_COUNT);
		}

		bool Init(SIZE_T InBaseIndex, SIZE_T InMinBaseBlockSize, 
			SIZE_T InMaxBaseBlockSize, SIZE_T InPoolBlockSize, SIZE_T InSubIndexCountShift);
		void Release();

		SIZE_T GetPoolCount();
		TMemPool* GetPool(SIZE_T Index);

	private:
		bool Initialized;

		SIZE_T  SubIndexCountShift;
		SIZE_T  MaxSubIndexCountShift;

		TMemPool Pools[MALLOC_SCALED_SUBINDEX_COUNT];
	};

	class TMemPoolTable
	{
		class TMemPoolTableStorage
		{
		public:
			TMemPoolTableStorage()
			{
				EntryCount = 0;
				FirstEntry = nullptr;
				LastEntry = nullptr;
			}

			bool Init(SIZE_T InEntryCount);

			TMemPoolTableEntry* GetFirst();
			TMemPoolTableEntry* GetLast();

			TMemPoolTableEntry* GetEntry(SIZE_T Index);
			SIZE_T GetEntryIndex(TMemPoolTableEntry* Entry);
			SIZE_T GetEntryCount();

			TMemPoolTableEntry& operator[](SIZE_T Index)
			{
				return FirstEntry[Index];
			}

			void Free();
			void Release();
		private:

			SIZE_T EntryCount;
			TMemPoolTableEntry* FirstEntry;
			TMemPoolTableEntry* LastEntry;

			FMallocScaledVMBlock DataBlock;
			static const SIZE_T EntrySize = sizeof(TMemPoolTableEntry);
		};

	public:
		struct TMemPoolTableStats
		{
			TMemPoolTableStats()
			{
				Used = 0;
				PeakUsed = 0;
				PeakUsedBlockCount = 0;
				UsedBlockCount = 0;
				TotalBlockCount = 0;
				AllocatedSize = 0;
			}

			SIZE_T Used;
			SIZE_T PeakUsed;

			SIZE_T UsedBlockCount;
			SIZE_T PeakUsedBlockCount;
			SIZE_T TotalBlockCount;

			SIZE_T  AllocatedSize;
		};

		TMemPoolTable()
		{
			PoolBlockSize = 0;
			SubIndexCount = 0;
			SubIndexCountShift = 0;
			MinBaseIndex = 0;
			MinBaseBlockSize = 0;
			MaxBaseIndex = 0;
			MaxBaseBlockSize = 0;
		}

		SIZE_T GetPoolBlockSize();
		SIZE_T GetEntryCount();
		SIZE_T GetSubIndexCount();
		SIZE_T GetSubIndexCountShift();
		SIZE_T GetMinBaseBlockSize();
		SIZE_T GetMaxBaseBlockSize();
		SIZE_T GetMinBaseIndex();
		SIZE_T GetMaxBaseIndex();

		TMemPoolTableEntry* GetEntry(SIZE_T EntryNum);

		static SIZE_T CalculateNumOfBaseEntries(SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize);
		static SIZE_T CalculatePoolBlockSize(SIZE_T BaseIndex, SIZE_T PoolIndex, SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize, uint32 PoolIndexCount);

		static bool GetBaseIndex(SIZE_T BlockSize, SIZE_T MinBaseIndex, SIZE_T MaxBaseIndex, SIZE_T& OutBaseIdx);
		static SIZE_T GetPoolIndex(SIZE_T BlockSize, SIZE_T BaseIndex, SIZE_T MinBaseIndex, SIZE_T SubIndexCountShift);

		bool Init(SIZE_T InMinBaseBlockSize, SIZE_T InMaxBaseBlockSize, SIZE_T InPoolBlockSize, SIZE_T InSubIndexCountShift);
		void Release();

	private:

		SIZE_T PoolBlockSize;
		SIZE_T SubIndexCount;
		SIZE_T SubIndexCountShift;
		SIZE_T MinBaseIndex;
		SIZE_T MinBaseBlockSize;

		SIZE_T MaxBaseIndex;
		SIZE_T MaxBaseBlockSize;

		TMemPoolTableStorage BaseEntries;

		void UpdateStats();

#if MALLOC_SCALED_MALLOC_STATS
		TMemPoolTableStats Stats;
#endif
	};

	FMallocScaledImpl()
	{
		Initialized = false;
	}

	~FMallocScaledImpl()
	{
	}

	void* Malloc(SIZE_T Size, uint32 Alignment);
	void* Realloc(void* Addr, SIZE_T NewSize, uint32 NewAlignment);
	void  Free(void* Addr);
	SIZE_T GetSize(void* Addr);

	bool Init();
	bool IsInitialized();
	void Shutdown();//Destroy();
	SIZE_T GetMallocMaxAlignment();
	void GetSpecificStats(void* OutStatData);

	SIZE_T GetBaseEntryCount();
	SIZE_T GetBlockSize(SIZE_T BaseIndex, SIZE_T PoolIndex);
	SIZE_T GetBlockCount(SIZE_T BaseIndex, SIZE_T PoolIndex);

	void DebugInit(SIZE_T MinBaseBlockSize, SIZE_T MaxBaseBlockSize, SIZE_T PoolBlockSize, uint32 SubIndexCount);

private:
	void* MallocInternal(SIZE_T Size, uint32 Alignment);
	void* ReallocInternal(void* Addr, SIZE_T NewSize, uint32 NewAlignment);
	void  FreeInternal(void* Addr);
	SIZE_T GetSizeInternal(void* Addr);

	bool Initialized;
	TMemPoolTable PoolTable;
	FCriticalSection CriticalSection;


};


class FMallocScaled final
	: public FMalloc
{
public:
	FMallocScaled();

	~FMallocScaled();


	virtual void* Malloc(SIZE_T Size, uint32 Alignment) override;
	virtual void* TryMalloc(SIZE_T Size, uint32 Alignment) override;
	virtual void* Realloc(void* Ptr, SIZE_T NewSize, uint32 NewAlignment) override;
	virtual void* TryRealloc(void* Ptr, SIZE_T NewSize, uint32 NewAlignment) override;
	virtual void Free(void* Ptr) override;
	virtual bool GetAllocationSize(void* Original, SIZE_T& SizeOut) override;

	virtual bool IsInternallyThreadSafe() const override;

	virtual bool ValidateHeap() override;

	virtual const TCHAR* GetDescriptiveName() override
	{
		return TEXT("MallocScaled");
	}

private:

	FMallocScaledImpl Impl;
};



