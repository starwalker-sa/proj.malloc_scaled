
#include "lib_malloc.h"
#include "mem_allocator.h"



TMemoryAllocator* TMemoryAllocator::GMemoryAllocator = nullptr;
TMallocBase* TMemoryAllocator::GMalloc = nullptr;
EMAllocToUse TMemoryAllocator::MallocToUse = EMAllocToUse::None;

static TMallocScaled GMallocScaled1;

IMalloc* TMemoryAllocator::GetMalloc()
{
	if (GMalloc)
	{
		if (GMalloc->IsInitialized())
		{
			return GMalloc;
		}
	}

	return nullptr;
}

bool TMemoryAllocator::Init(EMAllocToUse MallocToUse)
{
	switch (MallocToUse)
	{
	case MallocScaled1:
		GMalloc = &GMallocScaled1;
		TMemoryAllocator::MallocToUse = EMAllocToUse::MallocScaled1;
		break;
	case MallocScaled2:
		break;
	default:
		return false;
	}

	bool Ok = GMalloc->Init();

	return Ok;
}

TMallocScaled* TMemoryAllocator::GetMallocObject()
{
	switch (MallocToUse)
	{
	case MallocScaled1:
		MallocToUse = EMAllocToUse::MallocScaled1;
		return &GMallocScaled1;
	case MallocScaled2:
		break;
	default:
		break;
	}

	return nullptr;
}

TMemoryAllocator* TMemoryAllocator::GetMemoryAllocator()
{
	return GMemoryAllocator;
}

void TMemoryAllocator::Shutdown()
{
	if (GMalloc)
	{
		GMalloc->Shutdown();
	}
}

bool InitMalloc()
{
	return TMemoryAllocator::Init(EMAllocToUse::MallocScaled1);
}

bool IsMallocInitialized()
{
	return TMemoryAllocator::GetMallocObject()->IsInitialized();
}

void ShutdownMalloc()
{
	TMemoryAllocator::Shutdown();
}

void* Malloc(TSize Size, TSize Alignment)
{
	IMalloc* Ma = TMemoryAllocator::GetMalloc();

	if (Ma)
	{
		return Ma->Malloc(Size, Alignment);
	}

	return nullptr;
}

void* Realloc(void* Addr, TSize NewSize, TSize NewAlignment)
{
	IMalloc* Ma = TMemoryAllocator::GetMalloc();

	if (Ma)
	{
		return Ma->Realloc(Addr, NewSize, NewAlignment);
	}

	return nullptr;
}

void  Free(void* Addr)
{
	IMalloc* Ma = TMemoryAllocator::GetMalloc();

	if (Ma)
	{
		Ma->Free(Addr);
	}
}

TSize GetSize(void* Addr)
{
	IMalloc* Ma = TMemoryAllocator::GetMalloc();

	if (Ma)
	{
		return Ma->GetSize(Addr);
	}

	return 0;
}

//TMallocScaled* GetMallocObject(EMAllocToUse MallocToUse)
//{
//
//}

void GetMallocStats(TMallocStats& Stats)
{
	TMallocScaled* MemoryAllocator = TMemoryAllocator::GetMallocObject();
	if (MemoryAllocator)
	{
		MemoryAllocator->GetMallocStats(Stats);
	}

	
}

void GetMallocTimeStats(TMallocTimeStats& TimeStats)
{
	TMallocScaled* MemoryAllocator = TMemoryAllocator::GetMallocObject();
	if (MemoryAllocator)
	{
		MemoryAllocator->GetMallocTimeStats(TimeStats);
	}

}

TSize GetMaxPoolBlockSize()
{
	TMallocScaled* MemoryAllocator = TMemoryAllocator::GetMallocObject();
	if (MemoryAllocator)
	{
		return MemoryAllocator->GetMaxPoolBlockSize();
	}

	return 0;
}

float64 GetAvgTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act)
{
	std::chrono::duration<float64, std::milli> Time = {};
	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetAvgTime();
		return Time.count();
		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetAvgTime();
		return Time.count();
		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetAvgTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

float64 GetMaxTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act)
{
	std::chrono::duration<float64, std::milli> Time = {};

	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetMaxTime();
		return Time.count();
		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetMaxTime();
		return Time.count();
		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetMaxTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

float64 GetMinTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act)
{
	std::chrono::duration<float64, std::milli> Time = {};
	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetMinTime();
		return Time.count();
		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetMinTime();
		return Time.count();
		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetMinTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

float64 GetAvgTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act)
{

	std::chrono::duration<float64, std::nano> Time = {};
	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetAvgTime();
		return Time.count();
		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetAvgTime();
		return Time.count();
		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetAvgTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

float64 GetMaxTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act)
{
	std::chrono::duration<float64, std::nano> Time = {};
	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetMaxTime();
		return Time.count();
		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetMaxTime();
		return Time.count();
		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetMaxTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

float64 GetMinTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act)
{
	std::chrono::duration<float64, std::nano> Time = {};
	switch (Act)
	{
	case MALLOC:
		Time = TimeStats.BlockAllocTime.GetMinTime();
		return Time.count();

		break;
	case REALLOC:
		Time = TimeStats.BlockReallocTime.GetMinTime();
		return Time.count();

		break;
	case FREE:
		Time = TimeStats.BlockFreeTime.GetMinTime();
		return Time.count();
		break;
	default:
		break;
	}

	return 0.0;
}

uint64 GetMeasureCount(TMallocTimeStats& TimeStats, EMallocAction Act)
{
	switch (Act)
	{
	case MALLOC:
		return TimeStats.BlockAllocTime.GetMeasureCount();
		break;
	case REALLOC:
		return TimeStats.BlockReallocTime.GetMeasureCount();
		break;
	case FREE:
		return TimeStats.BlockFreeTime.GetMeasureCount();
		break;
	default:
		break;
	}

	return 0;
}