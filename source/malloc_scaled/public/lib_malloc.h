#pragma once

#include "build.h"
#include "std.h"
#include "malloc_base.h"

enum EMallocAction
{
	MALLOC,
	REALLOC,
	FREE
};


#if PLATFORM_WIN

//#if defined(MALLOC_DEBUG) || defined(MALLOC_STATS) || defined(MALLOC_TIME_STATS)
extern "C" __declspec(dllexport) bool InitMalloc();
extern "C" __declspec(dllexport) bool IsMallocInitialized();
extern "C" __declspec(dllexport) void ShutdownMalloc();
extern "C" __declspec(dllexport) void GetMallocStats(TMallocStats& Stats);
extern "C" __declspec(dllexport) void GetMallocTimeStats(TMallocTimeStats & TimeStats);
extern "C" __declspec(dllexport) TSize GetMaxPoolBlockSize();

extern "C" __declspec(dllexport) float64 GetAvgTime_ms(TMallocTimeStats& TimeStats, EMallocAction Act);
extern "C" __declspec(dllexport) float64 GetMaxTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" __declspec(dllexport) float64 GetMinTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act);

extern "C" __declspec(dllexport) float64 GetAvgTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" __declspec(dllexport) float64 GetMaxTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" __declspec(dllexport) float64 GetMinTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" __declspec(dllexport) uint64 GetMeasureCount(TMallocTimeStats & TimeStats, EMallocAction Act);

//#endif

extern "C" __declspec(dllexport) void* Malloc(TSize Size, TSize Alignment = MALLOC_DEFAULT_ALIGNMENT);
extern "C" __declspec(dllexport) void* Realloc(void* Addr, TSize NewSize, TSize NewAlignment = MALLOC_DEFAULT_ALIGNMENT);
extern "C" __declspec(dllexport) void  Free(void* Addr);
extern "C" __declspec(dllexport) TSize GetSize(void* Addr);
extern "C" __declspec(dllexport) float64 GetFunctionTime();

#endif

#if PLATFORM_UNIX

#if defined(MALLOC_DEBUG) || defined(MALLOC_STATS) || defined(MALLOC_TIME_STATS)
extern "C" bool InitMalloc();
extern "C" void ShutdownMalloc();
extern "C" void GetMallocStats(TMallocStats & Stats);
extern "C" void GetMallocTimeStats(TMallocTimeStats & TimeStats);
extern "C" TSize GetMaxPoolBlockSize();

extern "C" float64 GetAvgTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" float64 GetMaxTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" float64 GetMinTime_ms(TMallocTimeStats & TimeStats, EMallocAction Act);

extern "C" float64 GetAvgTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" float64 GetMaxTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" float64 GetMinTime_ns(TMallocTimeStats & TimeStats, EMallocAction Act);
extern "C" uint64 GetMeasureCount(TMallocTimeStats & TimeStats, EMallocAction Act);
#endif

extern "C" void* Malloc(TSize Size, TSize Alignment);
extern "C" void* Realloc(void* Addr, TSize NewSize, TSize NewAlignment);
extern "C" void  Free(void* Addr);
extern "C" TSize GetSize(void* Addr);

#endif