// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
//#include "PlatformMemory.h"
#include "HAL/CriticalSection.h"
#include <chrono>

class IPageMalloc;

static constexpr int32 MALLOC_SCALED_MAX_ARENA_COUNT = 256;

enum class FVMBlockAccess
{
	NoAccess,
	ReadOnly,
	ReadWrite,
	Execute,
	FullAccess
};

class FMemoryBlock
{
public:
	FMemoryBlock() :
		Size(0),
		Base(nullptr)
	{

	}


	FMemoryBlock(void* Ptr, SIZE_T Size) :
		Size(Size),
		Base(Ptr)
	{

	}

	void* GetBase() const
	{
		return Base;
	}

	SIZE_T GetSize() const
	{
		return Size;
	}

	void* GetEnd() const
	{
		return (uint8*)Base + Size;
	}

private:
	SIZE_T Size;
	void* Base;
};

class FMallocScaledVMBlock
{
public:
	FMallocScaledVMBlock()
	{
		Allocated = false;
	}

	FMallocScaledVMBlock(FMallocScaledVMBlock&&) = default;
	FMallocScaledVMBlock& operator=(FMallocScaledVMBlock&&) = default;

	FMallocScaledVMBlock(FMallocScaledVMBlock&) = delete;
	FMallocScaledVMBlock& operator=(FMallocScaledVMBlock&) = delete;

	static bool Init();
	static void Release();

	bool Allocate(SIZE_T Size);
	bool Allocate(void* Address, SIZE_T Size); // !!! Reserve pages at specific address;
	void Free();

	bool SetProtection(void* Offset, SIZE_T Size, FVMBlockAccess AccessFlag);

	bool IsAllocated();
	static bool IsPagingSupported();
	static bool IsProtectionSupported();

	static SIZE_T GetPageSize();

	void* GetBase();
	void* GetEnd();

	SIZE_T GetAllocatedSize();

private:
	bool Allocated;
	FMemoryBlock VMBlock;

	//static SIZE_T InitArenaSize;
	static IPageMalloc* PageMalloc;
};

#define PAGE_MALLOC_DEBUG 0
#define PAGE_MALLOC_STATS 0
#define PAGE_MALLOC_TIME_STATS 0

typedef std::chrono::duration<std::chrono::high_resolution_clock::rep, std::nano> FMallocScaledDuration;

namespace FMallocScaledMath
{
	inline constexpr uint64 FloorLog2(uint64 x)
	{
		return x == 1 ? 0 : 1 + FloorLog2(x >> 1);
	}

	inline constexpr uint64 CeilLog2(uint64 x)
	{
		return x == 1 ? 0 : FloorLog2(x - 1) + 1;
	}

	template<class T>
	inline constexpr bool IsPow2(T Size)
	{
		return (Size & (Size - 1)) == 0;
	}

	const uint64 tab64[64] = {
		63,  0, 58,  1, 59, 47, 53,  2,
		60, 39, 48, 27, 54, 33, 42,  3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22,  4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16,  9, 12,
		44, 24, 15,  8, 23,  7,  6,  5 };

	inline constexpr uint64 Log2_64(uint64 value)
	{
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		value |= value >> 32;
		return tab64[((uint64)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
	}

	uint64 constexpr Pow2_64(uint64 x)
	{
		return x == 0 ? 1 : Pow2_64(x - 1) << 1;
	}
}

inline bool IsPartOf(const void* Ptr, const void* Start, SIZE_T Size)
{
	const void* End = static_cast<const uint8_t*>(Start) + Size;
	return (Ptr < End && Ptr >= Start);
}

class FMallocScaledTimeStats
{
public:
	FMallocScaledTimeStats() :
		MaxTime(0),
		MinTime(FMallocScaledDuration::max()),
		AvgTime(0),
		LastTime(0),
		TotalTime(0),
		MeasureCount(0)
	{

	}

	void Reset()
	{
		MaxTime = {};
		MinTime = FMallocScaledDuration::max();
		AvgTime = {};
		LastTime = {};
		TotalTime = {};

		MeasureCount = 0;
	}

	void AddInterval(FMallocScaledDuration TimeInterval)
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

	FMallocScaledTimeStats& operator+(FMallocScaledDuration TimeInterval)
	{
		AddInterval(TimeInterval);
		return *this;
	}

	FMallocScaledTimeStats& operator+=(FMallocScaledDuration TimeInterval)
	{
		return *this + TimeInterval;
	}

	FMallocScaledDuration GetMaxTime()
	{
		return MaxTime;
	}

	FMallocScaledDuration GetMinTime()
	{
		return MinTime;
	}

	FMallocScaledDuration GetAvgTime()
	{
		return AvgTime;
	}

	FMallocScaledDuration GetLastTime()
	{
		return LastTime;
	}

	FMallocScaledDuration GetTotalTime()
	{
		return TotalTime;
	}

	uint64    GetMeasureCount()
	{
		return MeasureCount;
	}

private:
	FMallocScaledDuration MaxTime;
	FMallocScaledDuration MinTime;
	FMallocScaledDuration AvgTime;
	FMallocScaledDuration LastTime;
	FMallocScaledDuration TotalTime;

	uint64 MeasureCount;
};

struct FPageMallocTimeStats
{
	FMallocScaledTimeStats AllocRequestTime;
	FMallocScaledTimeStats FreeRequestTime;
	FMallocScaledTimeStats ReserveRequestTime;
};

struct FPageMallocStats
{
	FPageMallocStats() :
		ArenaCount(0),
		MaxArenaCount(0),
		ArenaMaxSize(0),
		ArenaMinSize(std::numeric_limits<SIZE_T>::max()),
		ArenaPageSize(0),

		AllocRequests(0),
		FreeRequests(0),
		ReserveRequests(0),

		FailedAllocRequests(0),
		FailedFreeRequests(0),
		FailedReserveRequests(0),

		MaxBlockSizeToAlloc(0),
		MaxBlockSizeToFree(0),
		//MaxBlockSizeToReserve(0),

		TotalUsedSize(0),
		TotalReservedSize(0)
	{

	}

	SIZE_T ArenaCount;
	SIZE_T MaxArenaCount;
	SIZE_T ArenaMaxSize;
	SIZE_T ArenaMinSize;
	SIZE_T ArenaPageSize;

	SIZE_T AllocRequests;
	SIZE_T FreeRequests;
	SIZE_T ReserveRequests;

	SIZE_T FailedAllocRequests;
	SIZE_T FailedFreeRequests;
	SIZE_T FailedReserveRequests;

	SIZE_T MaxBlockSizeToAlloc;
	SIZE_T MaxBlockSizeToFree;
	//size_t MaxBlockSizeToReserve;

	SIZE_T TotalUsedSize;
	SIZE_T TotalReservedSize;
};





class IPageMalloc
{
public:
	virtual bool Init(SIZE_T ArenaPageSize, SIZE_T PageSize, SIZE_T ArenaMinSize) = 0;

	virtual bool AllocateBlock(SIZE_T Size, FMemoryBlock& OutBlock, void* ArenaBaseAddr = nullptr) = 0;
	virtual bool AllocateBlock(void* Address, SIZE_T Size, FMemoryBlock& OutBlock) = 0;

	virtual bool FreeBlock(FMemoryBlock Block) = 0;
	virtual bool Reserve(FMemoryBlock& OutBlock) = 0;
	virtual bool Reserve(SIZE_T Size, FMemoryBlock& OutBlock) = 0;

	virtual bool SetProtection(FMemoryBlock Block, FVMBlockAccess Protect) = 0;

	virtual bool IsProtectionSupported() = 0;

	virtual SIZE_T GetGranularity() = 0;
	virtual SIZE_T GetPageSize() = 0;

	virtual bool Free() = 0;
	virtual bool Release() = 0;
	virtual bool Release(void* ArenaBaseAddr) = 0;

	virtual void GetStats(FPageMallocStats&) = 0;
	virtual void GetLastTimeStats(FPageMallocTimeStats&) = 0;

	virtual ~IPageMalloc() = default;
};

static const SIZE_T PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT = 16;

class FMallocScaledPageMalloc :
	public IPageMalloc
{


	template<typename TELEM>
	class FListNode
	{
	public:
		FListNode() :
			Next(nullptr),
			Prev(nullptr),
			Element{}
		{

		}

		FListNode(const TELEM& Elem) :
			Element(Elem),
			Next(nullptr),
			Prev(nullptr)
		{
		}

		FListNode(TELEM&& Elem) :
			Element(std::move(Elem)),
			Next(nullptr),
			Prev(nullptr)
		{
		}

		FListNode& operator=(const FListNode& Src)
		{
			if (&Src != this)
			{
				Element = Src.Element;
			}

			return *this;
		}

		FListNode& operator=(FListNode&& Src)
		{
			if (&Src != this)
			{
				Element = move(Src.Element);
			}

			return *this;
		}

		void LinkAfter(FListNode* Node)
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

		void LinkBefore(FListNode* Node)
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

		void LinkReplace(FListNode* Node)
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

		bool IsAdjecent(const FListNode* Node) const
		{
			return (Node &&
				(Next == Node && Node->Prev == this ||
					Prev == Node && Node->Next == this));
		}

		FListNode* GetNext() const
		{
			return Next;
		}

		FListNode* GetPrev() const
		{
			return Prev;
		}

		void SetNext(FListNode* Node)
		{
			Next = Node;
		}

		void SetPrev(FListNode* Node)
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

		~FListNode()
		{

		}

	private:
		TELEM Element;
		FListNode* Next;
		FListNode* Prev;
	};



	template<typename TNODE>
	class FListBase
	{
	public:
		using TNode = TNODE; // FList::FListNode<TELEM>;

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

		FListBase() :
			FirstNode(nullptr),
			LastNode(nullptr)
		{

		}

		FListBase(TNode* First, TNode* Last) :
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

		void SwapLists(FListBase& Src)
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

		TNode* GetLastNode(TNode* First, uint32 Count)
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

	class alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		FArena
	{
		enum FBlockState : SIZE_T
		{
			ALLOCATED,
			RELEASED
		};

		struct FBlock;
		using FBlockBase = FListNode<FBlock*>;

		struct alignas (PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT) FBlock :
			public FBlockBase
		{
			FBlock() :
				FBlockBase(this),
				Upper(nullptr),
				Lower(nullptr),
				State(RELEASED),
				Size(0),
				Ptr(nullptr)
			{
			}

			FBlock* Upper;
			FBlock* Lower;
			FBlockState State;
			SIZE_T Size;
			void* Ptr;
		};

		using FBlockList = FListBase<FBlockBase>;

		class FArenaPageTable
		{
		public:
			FArenaPageTable();

			bool Init(SIZE_T ArenaSize, SIZE_T ArenaPageSizeShift);

			inline FBlock* GetFirst();
			inline FBlock* GetLast();

			inline FBlock* GetArenaPage(SIZE_T Index);
			inline SIZE_T GetArenaPageIndex(FBlock* Node);
			inline SIZE_T GetArenaPageCount();

			void Free();
			bool Release();

		private:
			SIZE_T	PageCount;
			FBlock* FirstPage;
			FBlock* LastPage;

			FMemoryBlock DataBlock;
			static constexpr SIZE_T BlockTypeSize = sizeof(FBlock);
		};

	public:
		FArena();

		bool Init(SIZE_T ArenaSize, SIZE_T ArenaPageSize, SIZE_T PageSize,
			FPageMallocTimeStats* OutTimeStats = nullptr);

		inline void* TryMallocBlock(SIZE_T Size, SIZE_T& OutSize, void* Address);
		inline bool  TryFreeBlock(void* Address);

		inline SIZE_T GetArenaSize();
		inline void* GetArenaBase();
		inline SIZE_T GetArenaPageSize();
		inline bool IsEmpty();

		void Free();
		bool Release();

		bool IsInitialized();
#if	PAGE_MALLOC_DEBUG
		void DumpMemMap();
#endif
	private:
		inline void* AllocateBlock(FBlock* Block, SIZE_T CommitedSize);
		inline void* AllocateBlock(FBlock* Block, void* Addr, SIZE_T CommitedSize);

		inline FBlock* GetFreeBlock(SIZE_T AlignedSize);
		inline FBlock* GetFreeBlock(void* Address, SIZE_T AlignedSize);

		inline FBlock* SplitReleasedBlock(FBlock* ParentBlockNode, SIZE_T SizeToSplit);
		inline void MergeAdjecentReleasedBlocks(FBlock* BlockNode);
		inline bool IsPartOf(void* Addr, SIZE_T Size, void* LowerBorder, void* UpperBorder);

		SIZE_T RestFreeSize;
		SIZE_T FreeBlockCount;
		SIZE_T UserBlockCount;
		FBlockList FreeBlockList;
		FMemoryBlock Arena;

		FArenaPageTable ArenaPages;

		bool Initialized;
		SIZE_T ArenaPageSize;
		SIZE_T ArenaPageSizeShift;
		FPageMallocTimeStats* LastTimeStats;
	};
public:

	virtual bool Init(SIZE_T ArenaPageSize = 0, SIZE_T PageSize = 0, SIZE_T ArenaMinSize = 0);

	virtual bool AllocateBlock(SIZE_T Size, FMemoryBlock& OutBlock, void* ArenaBaseAddr = nullptr);
	virtual bool AllocateBlock(void* Address, SIZE_T Size, FMemoryBlock& OutBlock);
	virtual bool FreeBlock(FMemoryBlock Block);
	virtual bool Reserve(FMemoryBlock& OutBlock);
	virtual bool Reserve(SIZE_T Size, FMemoryBlock& OutBlock);

	virtual bool SetProtection(FMemoryBlock Block, FVMBlockAccess Access);

	virtual bool IsProtectionSupported();

	virtual SIZE_T GetGranularity();
	virtual SIZE_T GetPageSize();

	virtual bool Free();
	virtual bool Release();
	virtual bool Release(void* ArenaBaseAddr);

	static SIZE_T GetMaxArenaCount();

	void GetStats(FPageMallocStats&);
	void GetLastTimeStats(FPageMallocTimeStats&);

#if	PAGE_MALLOC_DEBUG
	static size_t GetArenaDefaultSize();
	void DumpMemMap(void* ArenaBaseAddr);
#endif

	static FMallocScaledPageMalloc* GetPageMalloc();

private:
	FMallocScaledPageMalloc();

	enum EInvalidSlot : int32
	{
		INVALID_SLOT = -1
	};

	void* TryAllocateBlock(SIZE_T Size, SIZE_T& OutSize, void* ArenaBaseAddr);
	void* TryAllocateBlock(void* Address, SIZE_T Size, SIZE_T& OutSize);

	struct alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		FArenaSlot
	{
		FArenaSlot()
		{
			Free = true;
		}

		bool Free;
		FArena Arena;
	};

	SIZE_T ArenaPageSize;
	SIZE_T PageSize;
	SIZE_T ArenaMinSize;

	FCriticalSection Guard;

	FPageMallocStats Stats;
	FPageMallocTimeStats LastTimeStats;


	class alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		FArenaTable
	{
	public:
		FArenaTable()
		{
			ArenaCount = 0;
			FirstArena = nullptr;
			LastArena = nullptr;
		}

		bool Init(SIZE_T ArenaCount);

		SIZE_T GetArenaSlotIndex(FArenaSlot* Node);
		SIZE_T GetArenaSlotCount();

		FArenaSlot* operator[](SIZE_T Index)
		{
			if (Index < ArenaCount)
			{
				return &FirstArena[Index];
			}

			return nullptr;
		}

		int32 GetFreeSlot();

		void Free();
		bool Release();

	private:
		SIZE_T	ArenaCount;
		FArenaSlot* FirstArena;
		FArenaSlot* LastArena;

		FMemoryBlock DataBlock;
		static constexpr SIZE_T ArenaSlotSize = sizeof(FArenaSlot);
	};

	FArenaTable ArenaTable;
	static FMallocScaledPageMalloc* GPageMalloc;
};

namespace FMallocScaledUtils
{
	template<typename TELEM>
	void CreateElementsDefault(TELEM* Dst, SIZE_T Count)
	{
		while (Count)
		{
			new (Dst) TELEM{};
			++Dst;
			--Count;
		}
	}
}