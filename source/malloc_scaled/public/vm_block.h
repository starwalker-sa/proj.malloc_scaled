#pragma once

#include "platform_memory.h"
#include "list_base.h"
#include "critical_section.h"
#include "time_stats.h"
#include "mem_block.h"

class IPageMalloc;

static constexpr int32 PAGE_MALLOC_MAX_ARENA_COUNT = 256;

class TVMBlock
{
public:
	TVMBlock()
	{
		Allocated = false;
	}

	TVMBlock(TVMBlock&&) = default;
	TVMBlock& operator=(TVMBlock&&) = default;

	TVMBlock(TVMBlock&) = delete;
	TVMBlock& operator=(TVMBlock&) = delete;

	static bool Init();
	static void Release();

	bool Allocate(TSize Size);
	bool Allocate(void* Address, TSize Size); // !!! Reserve pages at specific address;
	void Free();

	bool SetProtection(void* Offset, TSize Size, TMemoryBlockAccess AccessFlag);

	bool IsAllocated();
	static bool IsPagingSupported();
	static bool IsProtectionSupported();

	static TSize GetPageSize();

	void* GetBase();
	void* GetEnd();

	TSize GetAllocatedSize();

private:
	bool Allocated;
	TMemoryBlock VMBlock;

	//static TSize InitArenaSize;
	static IPageMalloc* PageMalloc;
};

#define PAGE_MALLOC_DEBUG 0
#define PAGE_MALLOC_STATS 0
#define PAGE_MALLOC_TIME_STATS 0


struct TPageMallocTimeStats
{
	TTimeStats AllocRequestTime;
	TTimeStats FreeRequestTime;
	TTimeStats ReserveRequestTime;
};

struct TPageMallocStats
{
	TPageMallocStats() :
		ArenaCount(0),
		MaxArenaCount(0),
		ArenaMaxSize(0),
		ArenaMinSize(std::numeric_limits<TSize>::max()),
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

	TSize ArenaCount;
	TSize MaxArenaCount;
	TSize ArenaMaxSize;
	TSize ArenaMinSize;
	TSize ArenaPageSize;

	TSize AllocRequests;
	TSize FreeRequests;
	TSize ReserveRequests;

	TSize FailedAllocRequests;
	TSize FailedFreeRequests;
	TSize FailedReserveRequests;

	TSize MaxBlockSizeToAlloc;
	TSize MaxBlockSizeToFree;
	//size_t MaxBlockSizeToReserve;

	TSize TotalUsedSize;
	TSize TotalReservedSize;
};

class IPageMalloc
{
public:
	virtual bool Init(IPlatformMalloc* PlatformMalloc, TSize ArenaPageSize, TSize PageSize, TSize ArenaMinSize) = 0;

	virtual bool AllocateBlock(TSize Size, TMemoryBlock& OutBlock, void* ArenaBaseAddr = nullptr) = 0;
	virtual bool AllocateBlock(void* Address, TSize Size, TMemoryBlock& OutBlock) = 0;

	virtual bool FreeBlock(TMemoryBlock Block) = 0;
	virtual bool Reserve(TMemoryBlock& OutBlock) = 0;
	virtual bool Reserve(TSize Size, TMemoryBlock& OutBlock) = 0;

	virtual bool SetProtection(TMemoryBlock Block, TMemoryBlockAccess Protect) = 0;

	virtual bool IsProtectionSupported() = 0;

	virtual TSize GetGranularity() = 0;
	virtual TSize GetPageSize() = 0;

	virtual bool Free() = 0;
	virtual bool Release() = 0;
	virtual bool Release(void* ArenaBaseAddr) = 0;

	virtual void GetStats(TPageMallocStats&) = 0;
	virtual void GetLastTimeStats(TPageMallocTimeStats&) = 0;

	virtual ~IPageMalloc() = default;
};

static const TSize PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT = 16;

class TPageMalloc :
	public IPageMalloc
{
	class alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		TArena
	{
		enum TBlockState : TSize
		{
			ALLOCATED,
			RELEASED
		};

		struct TBlock;
		using TBlockBase = TListNode<TBlock*>;

		struct alignas (PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT) TBlock :
			public TBlockBase
		{
			TBlock() :
				TBlockBase(this),
				Upper(nullptr),
				Lower(nullptr),
				State(RELEASED),
				Size(0),
				Ptr(nullptr)
			{
			}

			TBlock* Upper;
			TBlock* Lower;
			TBlockState State;
			TSize Size;
			void* Ptr;
		};

		using TBlockList = TListBase<TBlockBase>;

		class TArenaPageTable
		{
		public:
			TArenaPageTable();

			bool Init(IPlatformMalloc* PMalloc, TSize ArenaSize, TSize ArenaPageSizeShift);

			inline TBlock* GetFirst();
			inline TBlock* GetLast();

			inline TBlock* GetArenaPage(TSize Index);
			inline TSize GetArenaPageIndex(TBlock* Node);
			inline TSize GetArenaPageCount();

			void Free();
			bool Release();

		private:
			TSize	PageCount;
			TBlock* FirstPage;
			TBlock* LastPage;

			IPlatformMalloc* PlatformMalloc;
			TPlatformMemoryBlock DataBlock;
			static constexpr TSize BlockTypeSize = sizeof(TBlock);
		};

	public:
		TArena();

		bool Init(TSize ArenaSize, TSize ArenaPageSize, TSize PageSize,
			IPlatformMalloc* PMalloc, TPageMallocTimeStats* OutTimeStats = nullptr);

		inline void* TryMallocBlock(TSize Size, TSize& OutSize, void* Address);
		inline bool  TryFreeBlock(void* Address);

		inline TSize GetArenaSize();
		inline void* GetArenaBase();
		inline TSize GetArenaPageSize();
		inline bool IsEmpty();

		void Free();
		bool Release();

		bool IsInitialized();
#if	PAGE_MALLOC_DEBUG
		void DumpMemMap();
#endif
	private:
		inline void* AllocateBlock(TBlock* Block, TSize CommitedSize);
		inline void* AllocateBlock(TBlock* Block, void* Addr, TSize CommitedSize);

		inline TBlock* GetFreeBlock(TSize AlignedSize);
		inline TBlock* GetFreeBlock(void* Address, TSize AlignedSize);

		inline TBlock* SplitReleasedBlock(TBlock* ParentBlockNode, TSize SizeToSplit);
		inline void MergeAdjecentReleasedBlocks(TBlock* BlockNode);
		inline bool IsPartOf(void* Addr, TSize Size, void* LowerBorder, void* UpperBorder);

		TSize RestFreeSize;
		TSize FreeBlockCount;
		TSize UserBlockCount;
		TBlockList FreeBlockList;
		TPlatformMemoryBlock Arena;

		TArenaPageTable ArenaPages;

		bool Initialized;
		TSize ArenaPageSize;
		TSize ArenaPageSizeShift;
		IPlatformMalloc* PlatformMalloc;
		TPageMallocTimeStats* LastTimeStats;
	};
public:

	virtual bool Init(IPlatformMalloc* PlatformMalloc, TSize ArenaPageSize = 0, TSize PageSize = 0, TSize ArenaMinSize = 0);

	virtual bool AllocateBlock(TSize Size, TMemoryBlock& OutBlock, void* ArenaBaseAddr = nullptr);
	virtual bool AllocateBlock(void* Address, TSize Size, TMemoryBlock& OutBlock);
	virtual bool FreeBlock(TMemoryBlock Block);
	virtual bool Reserve(TMemoryBlock& OutBlock);
	virtual bool Reserve(TSize Size, TMemoryBlock& OutBlock);

	virtual bool SetProtection(TMemoryBlock Block, TMemoryBlockAccess Access);

	virtual bool IsProtectionSupported();

	virtual TSize GetGranularity();
	virtual TSize GetPageSize();

	virtual bool Free();
	virtual bool Release();
	virtual bool Release(void* ArenaBaseAddr);

	static TSize GetMaxArenaCount();

	void GetStats(TPageMallocStats&);
	void GetLastTimeStats(TPageMallocTimeStats&);

#if	PAGE_MALLOC_DEBUG
	static size_t GetArenaDefaultSize();
	void DumpMemMap(void* ArenaBaseAddr);
#endif

	static TPageMalloc* GetPageMalloc();

private:
	TPageMalloc();

	enum TInvalidSlot : int32_t
	{
		INVALID_SLOT = -1
	};

	void* TryAllocateBlock(TSize Size, TSize& OutSize, void* ArenaBaseAddr);
	void* TryAllocateBlock(void* Address, TSize Size, TSize& OutSize);

	struct alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		TArenaSlot
	{
		TArenaSlot()
		{
			Free = true;
		}

		bool Free;
		TArena Arena;
	};

	TSize ArenaPageSize;
	TSize PageSize;
	TSize ArenaMinSize;

	IPlatformMalloc* PlatformMalloc;

	TCriticalSection Guard;

	TPageMallocStats Stats;
	TPageMallocTimeStats LastTimeStats;
	

	class alignas(PAGE_MALLOC_SYSTEM_DEFAULT_ALIGNMENT)
		TArenaTable
	{
	public:
		TArenaTable()
		{
			ArenaCount = 0;
			FirstArena = nullptr;
			LastArena = nullptr;
			PlatformMalloc = nullptr;
		}

		bool Init(IPlatformMalloc* PMalloc, TSize ArenaCount);

		TSize GetArenaSlotIndex(TArenaSlot* Node);
		TSize GetArenaSlotCount();

		TArenaSlot* operator[](TSize Index)
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
		TSize	ArenaCount;
		TArenaSlot* FirstArena;
		TArenaSlot* LastArena;

		IPlatformMalloc* PlatformMalloc;
		TPlatformMemoryBlock DataBlock;
		static constexpr TSize ArenaSlotSize = sizeof(TArenaSlot);
	};

	TArenaTable ArenaTable;
	static TPageMalloc* GPageMalloc;
};