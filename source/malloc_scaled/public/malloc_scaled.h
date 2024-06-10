#pragma once

#include "std.h"
#include "list_base.h"
#include "mem_block.h"
#include "vm_block.h"
#include "malloc_base.h"
#include "critical_section.h"

#include "align.h"

static const TSize MALLOC_SCALED_DEFAULT_ALIGNMENT        = 16;
static const TSize MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT = 16;
static const TSize MALLOC_SCALED_SUBINDEX_COUNT           = 8;
static const TSize MALLOC_SCALED_MAX_SUBINDEX_COUNT       = 16;
static const TSize MALLOC_SCALED_MIN_BASE_BLOCK_SIZE      = 256;        // Bytes;
static const TSize MALLOC_SCALED_MAX_BASE_BLOCK_SIZE      = 34359738368; // Bytes;
static const TSize MALLOC_SCALED_POOL_BLOCK_SIZE          = 8388608;   // Previous val: 524288 Bytes;
static const TSize MALLOC_SCALED_AREA_BLOCK_SIZE          = 268435456; // Bytes;


static_assert(IsPow2(MALLOC_SCALED_DEFAULT_ALIGNMENT),        "MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT must be power of 2");
static_assert(IsPow2(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT must be power of 2");
static_assert(IsPow2(MALLOC_SCALED_MAX_SUBINDEX_COUNT),       "MALLOC_SCALED_MAX_SUBINDEX_COUNT must be power of 2");
static_assert(IsPow2(MALLOC_SCALED_SUBINDEX_COUNT),           "MALLOC_SCALED_SUBINDEX_COUNT must be power of 2");
static_assert(IsPow2(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE),      "MALLOC_SCALED_MIN_BASE_BLOCK_SIZE must be power of 2");
static_assert(IsPow2(MALLOC_SCALED_MAX_BASE_BLOCK_SIZE),      "MALLOC_SCALED_MAX_BASE_BLOCK_SIZE must be power of 2");
static_assert(IsAligned(MALLOC_SCALED_MIN_BASE_BLOCK_SIZE / MALLOC_SCALED_SUBINDEX_COUNT, MALLOC_SCALED_DEFAULT_ALIGNMENT), "the smallest block size must be aligned by MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT");
static_assert(IsAligned(MALLOC_SCALED_POOL_BLOCK_SIZE, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "commited pool size must be aligned by MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT");

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
	TMemBlockHdr :
	public TMemBlockHdrBase
{
	TMemBlockHdr() :
		TMemBlockHdrBase(this)
	{
		UsedSize  = 0;
		BlockSize = 0;
		PoolHdr   = nullptr;
	}

	friend bool operator<(TMemBlockHdr& BlockA, TMemBlockHdr& BlockB)
	{
		return BlockA.UsedSize < BlockB.UsedSize;
	}

	friend bool operator>(TMemBlockHdr& BlockA, TMemBlockHdr& BlockB)
	{
		return BlockA.UsedSize > BlockB.UsedSize;
	}

	TSize UsedSize;  // !!! UsedSize = BlockSize - RestFreeSize;
	TSize BlockSize; // !!! Size of block without block header;
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
#ifdef MALLOC_STATS
		Used = 0;
#endif
		ActiveBlocks    = 0;
		TotalBlockCount = 0;
		FreeBlockCount  = 0;

		MemPool = nullptr;
	}

#ifdef MALLOC_STATS
	TSize Used;
#endif

	TSize ActiveBlocks;
	TSize FreeBlockCount;
	TSize TotalBlockCount;

	TMemPool* MemPool;
	TMemBlockList FreeBlockList;

#ifdef MALLOC_STATS
	TMemBlockList UsrBlockList;
#endif

	TVMBlock PoolVMBlock;
};

using TMemPoolList = TListBase<TMemPoolHdrBase>;

static const TSize MemBlockHdrOffsetSize = sizeof(TMemBlockHdrOffset);
static const TSize MemBlockHdrSize       = sizeof(TMemBlockHdr);
static const TSize MemPoolHdrSize        = sizeof(TMemPoolHdr);

static_assert(IsAligned(MemPoolHdrSize, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "Memory pool header size must be aligned by MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT");
static_assert(IsAligned(MemBlockHdrSize, MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT), "Memory block header size must be aligned by MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT");

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

		TSize Used;
		TSize PeakUsed;

		TSize UsedBlockCount;
		TSize PeakUsedBlockCount;
		TSize FreeBlockCount;
		TSize TotalBlockCount;

		TSize BlockSize;
		TSize AllocatedSize;

		TBlockStats BlockStats;
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

	void Init(TSize BaseIndex, TSize PoolIndex, TSize BlockSize, TSize PoolBlockSize);
	void Release();

	TMemBlockHdr* GetFreeBlock(TSize UsedSize);
	
	void FreeUsrBlock(TMemBlockHdr* UsrBlock);

	TSize GetBlockSize();
	TSize GetPoolCount();
	TMemPoolHdr* GetTop();

	TMemPoolStats* GetPoolStats();
private:
	TMemPoolHdr* FindNewHeadPool();

	TMemPoolHdr* AddPool();
	void DeletePool(TMemPoolHdr*);

	TSize PoolIndex;
	TSize BaseIndex;
	TSize PoolCount;
	TSize TotalFreeBlockCount;

	TMemPoolHdr* HeadPool;

	TSize BlockSize;

	TSize PoolBlockSize;
	TSize PoolVMBlockSize;

	TMemPoolList PoolList;

#ifdef MALLOC_STATS
	TMemPoolStats Stats;
#endif
};

class alignas(MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT) 
	TMemPoolTableEntry
{
public:
	TMemPoolTableEntry()
	{
		Initialized      = false;
		SubIndexCountShift    = 0;
		MaxSubIndexCountShift = Log2_64(MALLOC_SCALED_MAX_SUBINDEX_COUNT);
	}

	bool Init(TSize BaseIndex, TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize CommitedSize, TSize SubIndexCount);
	void Release();

	TSize GetPoolCount();
	TMemPool* GetPool(TSize Index);

private:
	bool Initialized;

	TSize SubIndexCountShift;
	TSize MaxSubIndexCountShift;

	TMemPool Pools[MALLOC_SCALED_MAX_SUBINDEX_COUNT];
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
			LastEntry  = nullptr;
		}

		bool Init(TSize EntryCount);

		TMemPoolTableEntry* GetFirst();
		TMemPoolTableEntry* GetLast();

		TMemPoolTableEntry* GetEntry(TSize Index);
		TSize GetEntryIndex(TMemPoolTableEntry* Entry);
		TSize GetEntryCount();

		TMemPoolTableEntry& operator[](TSize Index)
		{
			return FirstEntry[Index];
		}

		void Free();
		void Release();
	private:

		TSize EntryCount;
		TMemPoolTableEntry* FirstEntry;
		TMemPoolTableEntry* LastEntry;

		TVMBlock DataBlock;
		static const TSize EntrySize = sizeof(TMemPoolTableEntry);
	};

public:
	struct TMemPoolTableStats
	{
		TMemPoolTableStats()
		{
			Used     = 0;
			PeakUsed = 0;

			UsedBlockCount     = 0;
			PeakUsedBlockCount = 0;
			TotalBlockCount    = 0;

			AllocatedSize = 0;
		}

		TSize Used;
		TSize PeakUsed;

		TSize UsedBlockCount;
		TSize PeakUsedBlockCount;
		TSize TotalBlockCount;

		TSize  AllocatedSize;
	};

	TMemPoolTable()
	{
		PoolBlockSize = 0;
		SubIndexCount = 0;
		SubIndexCountShift = 0;
		MinBaseIndex  = 0;
		MinBaseBlockSize = 0;
		MaxBaseIndex = 0;
		MaxBaseBlockSize = 0;
	}

	TSize GetPoolBlockSize();
	TSize GetEntryCount();
	TSize GetSubIndexCount();
	TSize GetSubIndexCountShift();
	TSize GetMinBaseBlockSize();
	TSize GetMaxBaseBlockSize();
	TSize GetMinBaseIndex();
	TSize GetMaxBaseIndex();

	TMemPoolTableEntry* GetEntry(TSize EntryNum);

	static TSize CalculateNumOfBaseEntries(TSize MinBaseBlockSize, TSize MaxBaseBlockSize);
	static TSize CalculatePoolBlockSize(TSize BaseIndex, TSize PoolIndex, TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolIndexCount);

	static inline bool GetBaseIndex(TSize BlockSize, TSize MinBaseIndex, TSize MaxBaseIndex, TSize& OutBaseIdx);
	static inline TSize GetPoolIndex(TSize BlockSize, TSize BaseIndex, TSize MinBaseIndex, TSize PoolIndexCount);

	bool Init(TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolBlockSize, TSize SubIndexCount);
	void Release();

private:

	TSize PoolBlockSize;
	TSize SubIndexCount;
	TSize SubIndexCountShift;
	TSize MinBaseIndex;
	TSize MinBaseBlockSize;

	TSize MaxBaseIndex;
	TSize MaxBaseBlockSize;

	TMemPoolTableStorage BaseEntries;

	void UpdateStats();

#ifdef MALLOC_STATS
	TMemPoolTableStats Stats;
#endif
};

class TMallocScaled :
	public TMallocBase
{
public:
	TMallocScaled()
	{
		Initialized = false;
	}

	TMallocScaled(TMallocScaled&) = delete;
	TMallocScaled(TMallocScaled&&) = delete;

	TMallocScaled& operator=(TMallocScaled&) = delete;
	TMallocScaled& operator=(TMallocScaled&&) = delete;

	virtual void* Malloc(TSize Size, TSize Alignment = MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT) final;
	virtual void* Realloc(void* Addr, TSize NewSize, TSize NewAlignment = MALLOC_SCALED_SYSTEM_DEFAULT_ALIGNMENT) final;
	virtual void  Free(void* Addr) final;
	virtual TSize GetSize(void* Addr) final;

	virtual bool Init() final;
	virtual bool IsInitialized() final;
	virtual void Shutdown() final;

	virtual TSize GetMallocMaxAlignment() final;
	virtual void GetSpecificStats(void* OutStatData) final;

	TSize GetBaseEntryCount();
	TSize GetBlockSize(TSize BaseIndex, TSize PoolIndex);
	TSize GetBlockCount(TSize BaseIndex, TSize PoolIndex);
	TSize GetMaxPoolBlockSize();

	void DebugInit(TSize MinBaseBlockSize, TSize MaxBaseBlockSize, TSize PoolBlockSize, uint32 SubIndexCount);

private:
	inline void* MallocInternal(TSize Size, TSize Alignment);
	inline void* ReallocInternal(void* Addr, TSize Size, TSize Alignment);
	inline void  FreeInternal(void* Addr);
	TSize GetSizeInternal(void* Addr);

	bool Initialized;
	TMemPoolTable PoolTable;
	TCriticalSection Guard;
};


#define TIMER TTimer Timer;
#define FUNC_TIME(x)  \
	Timer.Start(); \
	x; \
	Timer.Stop();  \
	Ts += Timer.GetDuration(); 

//#define TIME_STOP() \
//do { \
//
//} while(0);


//float64 GetFunctionTime();