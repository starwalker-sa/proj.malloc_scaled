#pragma once

#include "std.h"
#include "time_stats.h"

/*
------------------------------------
	Global platform memory values
------------------------------------
*/

struct TMemoryGlobalStats
{
	TMemoryGlobalStats() :
		PageSize(0),
		UsedPhysicalMemory(0),
		UsedVirtualMemory(0),
		AvailPhysicalMemory(0),
		AvailVirtualMemory(0),
		TotalPhysicalMemory(0),
		TotalVirtualMemory(0),
		OsAllocGranularity(0)
	{
	}

	TSize  PageSize;
	uint64 UsedPhysicalMemory;
	uint64 UsedVirtualMemory;
	uint64 AvailPhysicalMemory;
	uint64 AvailVirtualMemory;
	uint64 TotalPhysicalMemory;
	uint64 TotalVirtualMemory;
	TSize  OsAllocGranularity;
};

/*
-------------------------------------
	Memory allocator generic stats
-------------------------------------
*/

struct TBlockStats
{
	TBlockStats() :
		MaxUsedBlock(0),
		MinUsedBlock(std::numeric_limits<TSize>::max()),
		LargestUsedBlock(0),
		SmallestUsedBlock(std::numeric_limits<TSize>::max())

	{

	}

	TSize MaxUsedBlock;
	TSize MinUsedBlock;
	TSize LargestUsedBlock;
	TSize SmallestUsedBlock;
};

struct TMallocStats
{
	struct TTotalStats
	{
		TTotalStats() :
			TotalUsed(0),
			TotalAllocated(0),
			TotalReserved(0),
			MaxTotalUsed(0),
			MaxTotalAllocated(0),
			MaxTotalReserved(0)
		{
		}

		TSize TotalUsed;     //~ 
		TSize TotalAllocated;//~
		TSize TotalReserved; // Allocated <= Reserved;

		/*
		*	Max peaks of memory used by allocator from all time since its startup
		*/

		TSize MaxTotalUsed;     //+
		TSize MaxTotalAllocated;//+
		TSize MaxTotalReserved; //+
	
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
			MinAllocRequestSize(std::numeric_limits<TSize>::max()),
			MaxFreeRequestSize(0),
			MinFreeRequestSize(std::numeric_limits<TSize>::max())
		{

		}

		TSize MaxAllocRequestSize;
		TSize MinAllocRequestSize;

		TSize MaxFreeRequestSize;
		TSize MinFreeRequestSize;

	} RequestVals;

	TBlockStats BlockStats;
};



struct TMallocTimeStats
{
	TTimeStats BlockAllocTime;
	TTimeStats BlockReallocTime;
	TTimeStats BlockFreeTime;
};
