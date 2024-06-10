#include "test_mthread_perf_malloc.h"
#include "malloc_stats.h"
#include "timer.h"
#include <chrono>
#include <cstdio>
//#include "intrin.h"
#include <mutex>
#include "platform.h"
#include <string>

static std::mutex InitGuard{};
static bool InitFlag = false;

bool SafeInitMalloc()
{
	InitGuard.lock();

	if (!InitFlag)
	{
		if (InitMalloc())
		{
			InitFlag = true;
		}
		else
		{
			if (IsMallocInitialized())
			{
				InitFlag = true;
			}
			else
			{
				InitGuard.unlock();
				return false;
			}
		}
	}

	InitGuard.unlock();
	return true;
}

void SafeShutdownMalloc()
{
	InitGuard.lock();
	ShutdownMalloc();
	InitFlag = false;
	InitGuard.unlock();
}

TLogger* GLogger = nullptr;

TLogger::TLogger(const char* FileName)
{
	Log = nullptr;
	
	Log = fopen(FileName, "a+");

	if (!Log)
	{
		printf("MALLOC PERF TEST: Cannot open log file. Exit\n");
		exit(EXIT_FAILURE);
	}
}

TLogger::~TLogger()
{
	fclose(Log);
}

TWorker::TTest TWorker::Tests[] = 
{
	{ TEST_MALLOC,  Test_Perf_Malloc_Const_Blocks_1},
	{ TEST_MALLOC,  Test_Perf_Malloc_Const_Blocks_2},
	//{ TEST_MALLOC,  Test_Perf_Malloc_Progressive_Blocks }, <== It's dangerous. Aggressive filling all memory : RAM and page file on disk!!
	{ TEST_FREE,    Test_Perf_Free },
	{ TEST_REALLOC, Test_Perf_Small_Reallocs },
	{ TEST_REALLOC, Test_Perf_Big_Reallocs }
};

std::atomic<uint32> TWorker::RunningTasks      = 0;
std::atomic<uint32> TWorker::AllTasksCompleted = true;
std::atomic<int32>  TWorker::ExitCode          = 0;

void ShowProgress(float64 Value, float64 MaxValue)
{
	float64 Completed = Value * 100.0f / MaxValue;
	printf("MALLOC PERF TEST: Tasks progress: %.2f %% completed\r", Completed);
}

void AggregateAndDumpStats(ETestType TestType, uint32 TestNumber);

void TWorker::Run()
{
#ifdef PLATFORM_WIN
	ThreadId = GetCurrentThreadId();
#endif
#ifdef PLATFORM_UNIX_BSD
	thr_self(&ThreadId);
#endif
#ifdef PLATFORM_LINUX
	ThreadId = getid();
#endif

	printf("MALLOC PERF TEST: Starting thread: %i; Executing tests...\n", ThreadId);

	for (uint32 i = 0; i < TestCount; ++i)
	{
		if (!SafeInitMalloc())
		{
			printf("MALLOC PERF TEST: PANIC!!! CANNOT INIT MALLOC. Program is terminated\n");
			ExitCode.store(EXIT_FAILURE);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		if (ExitCode.load() == EXIT_FAILURE)
		{
			break;
		}

		AllTasksCompleted = false;

		++RunningTasks;

		Tests[i].TestFunction(this);
		
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		--RunningTasks;

		if (RunningTasks.load() == 0)
		{
			if (ExitCode.load() == EXIT_FAILURE)
			{
				SafeShutdownMalloc();
				break;
			}

			printf("MALLOC PERF TEST: All tasks completed. I'am last! Thread %i are notifying others...\n", ThreadId);
			printf("MALLOC PERF TEST: Aggregating and dumping test results to file\n");
			
			AggregateAndDumpStats(Tests[i].TestType, i);
			//printf("find free block avg time is: %f ns\n", GetFunctionTime());
			//GLogger->DumpStrToFile(std::string("VM Block avg time is : " + std::to_string(GetFunctionTime()) + " ns\n").c_str());
			SafeShutdownMalloc();
			

			AllTasksCompleted = true;
			AllTasksCompleted.notify_all();
			
			
		}
		else
		{
			if (ExitCode.load() == EXIT_FAILURE)
			{
				break;
			}
			printf("MALLOC PERF TEST: Thread id: %i is waiting for others, task number: %i\n", ThreadId, i);

			// waiting for others;
			AllTasksCompleted.wait(false);
			
			
		}

		ResetStats();
	}

	if (ExitCode.load() == 0)
	{
		printf("MALLOC PERF TEST: Thread %i has finished all tests. Exit from thread function\n", ThreadId);
	}
	else
	{
		printf("MALLOC PERF TEST: Abnormal termination!\n ");
	}
}

void TLogger::DumpStatsToFile(
	const char* Header1,
	ETestType TestType, uint32 TestNumber,
	uint64 MesCount, float64 MinTime, float64 MaxTime, float64 AvgTime)
{
	Guard.lock();

	switch (TestType)
	{
	case TEST_MALLOC:
	{
		fprintf(Log, Header1);

		fprintf(Log, "Performance test results: Test number: %i\n", TestNumber);
		fprintf(Log, "Number of allocated memory blocks: %llu\n\n", MesCount);

		fprintf(Log, "Memory block allocation: max time : %f  ns;  %f  us\n", MaxTime, MaxTime / 1000.0f);
		fprintf(Log, "Memory block allocation: min time : %f  ns;  %f  us\n", MinTime, MinTime / 1000.0f);
		fprintf(Log, "Memory block allocation: avg time : %f  ns;  %f  us\n", AvgTime, AvgTime / 1000.0f);

		fprintf(Log, "==================================================================\n\n\n");
	}
		break;
	case TEST_REALLOC:
	{
		fprintf(Log, Header1);
		fprintf(Log, "Performance test results: Test number: %i\n", TestNumber);
		fprintf(Log, "Number of reallocated memory blocks : %llu\n", MesCount);
		fprintf(Log, "Memory block reallocation max time : %f  ns;  %f  us\n", MaxTime, MaxTime / 1000.0f);
		fprintf(Log, "Memory block reallocation min time : %f  ns;  %f  us\n", MinTime, MinTime / 1000.0f);
		fprintf(Log, "Memory block reallocation avg time : %f  ns;  %f  us\n", AvgTime, AvgTime / 1000.0f);

		fprintf(Log, "==================================================================\n\n\n");
	}
		break;
	case TEST_FREE:
	{
		fprintf(Log, Header1);
		fprintf(Log, "Performance test results: Test number: %i\n", TestNumber);
		fprintf(Log, "Number of released memory blocks : %llu\n", MesCount);
		fprintf(Log, "Memory block releasing max time : %f  ns;  %f  us\n", MaxTime, MaxTime / 1000.0f);
		fprintf(Log, "Memory block releasing min time : %f  ns;  %f  us\n", MinTime, MinTime / 1000.0f);
		fprintf(Log, "Memory block releasing avg time : %f  ns;  %f  us\n", AvgTime, AvgTime / 1000.0f);

		fprintf(Log, "==================================================================\n\n\n");
	}
		break;
	default:
		break;
	}
	Guard.unlock();
}

void TLogger::DumpStrToFile(const char* Str)
{
	fprintf(Log, "%s\n", Str);
}

void AggregateAndDumpStats(ETestType TestType, uint32 TestNumber)
{
	TSize WCount = Workers->size();

	uint64 MallocMesCount  = 0;
	uint64 ReallocMesCount = 0;
	uint64 FreeMesCount    = 0;

	float64 MallocAvgTime = 0.0f;
	float64 MallocMaxTime = 0.0f;
	float64 MallocMinTime = 0.0f;

	float64 ReallocAvgTime = 0.0f;
	float64 ReallocMaxTime = 0.0f;
	float64 ReallocMinTime = 0.0f;

	float64 FreeAvgTime = 0.0f;
	float64 FreeMaxTime = 0.0f;
	float64 FreeMinTime = 0.0f;

	for (uint32 i = 0; i < WCount; ++i)
	{
		auto DurMallocAvg  = (*Workers)[i]->MallocTimeStats.BlockAllocTime.GetAvgTime();
		auto DurMallocMin  = (*Workers)[i]->MallocTimeStats.BlockAllocTime.GetMinTime();
		auto DurMallocMax  = (*Workers)[i]->MallocTimeStats.BlockAllocTime.GetMaxTime();

		auto DurReallocAvg = (*Workers)[i]->MallocTimeStats.BlockReallocTime.GetAvgTime();
		auto DurReallocMin = (*Workers)[i]->MallocTimeStats.BlockReallocTime.GetMinTime();
		auto DurReallocMax = (*Workers)[i]->MallocTimeStats.BlockReallocTime.GetMaxTime();

		auto DurFreeAvg    = (*Workers)[i]->MallocTimeStats.BlockFreeTime.GetAvgTime();
		auto DurFreeMin    = (*Workers)[i]->MallocTimeStats.BlockFreeTime.GetMinTime();
		auto DurFreeMax    = (*Workers)[i]->MallocTimeStats.BlockFreeTime.GetMaxTime();

		MallocAvgTime  += std::chrono::duration<float64, std::nano>(DurMallocAvg).count();
		MallocMinTime  += std::chrono::duration<float64, std::nano>(DurMallocMin).count();
		MallocMaxTime  += std::chrono::duration<float64, std::nano>(DurMallocMax).count();

		ReallocAvgTime += std::chrono::duration<float64, std::nano>(DurReallocAvg).count();
		ReallocMinTime += std::chrono::duration<float64, std::nano>(DurReallocMin).count();
		ReallocMaxTime += std::chrono::duration<float64, std::nano>(DurReallocMax).count();

		FreeAvgTime    += std::chrono::duration<float64, std::nano>(DurFreeAvg).count();
		FreeMinTime    += std::chrono::duration<float64, std::nano>(DurFreeMin).count();
		FreeMaxTime    += std::chrono::duration<float64, std::nano>(DurFreeMax).count();

		MallocMesCount  += (*Workers)[i]->MallocTimeStats.BlockAllocTime.GetMeasureCount();
		ReallocMesCount += (*Workers)[i]->MallocTimeStats.BlockReallocTime.GetMeasureCount();
		FreeMesCount    += (*Workers)[i]->MallocTimeStats.BlockFreeTime.GetMeasureCount();
	}

	MallocAvgTime /= WCount;
	MallocMaxTime /= WCount;
	MallocMinTime /= WCount;

	ReallocAvgTime /= WCount;
	ReallocMaxTime /= WCount;
	ReallocMinTime /= WCount;

	FreeAvgTime /= WCount;
	FreeMaxTime /= WCount;
	FreeMinTime /= WCount;
	
	switch (TestType)
	{
	case TEST_MALLOC:
	{
		GLogger->DumpStatsToFile(
			"---------- MALLOC: ALLOCATIONS: SUMMARISED time stats ------------\n",
			TEST_MALLOC, TestNumber,
			MallocMesCount, MallocMinTime, MallocMaxTime, MallocAvgTime);
	}
		break;
	case TEST_REALLOC:
	{
		GLogger->DumpStatsToFile(
			"---------- MALLOC: REALLOCATIONS: SUMMARISED time stats ----------\n",
			TEST_REALLOC, TestNumber,
			ReallocMesCount, ReallocMinTime, ReallocMaxTime, ReallocAvgTime);
	}
		break;
	case TEST_FREE:
	{
		GLogger->DumpStatsToFile(
			"---------- MALLOC: RELEASES: SUMMARISED time stats ---------------\n",
			TEST_FREE, TestNumber,
			FreeMesCount, FreeMinTime, FreeMaxTime, FreeAvgTime);
	}
		break;
	default:
		break;
	}
}

void Test_Perf_Malloc_Const_Blocks_1(TWorker* Worker)
{
	void* Ptr = nullptr;
	TSize Size0 = 32;
	uint64 BlkCountLimit = 10000000;
	uint32 Id = Worker->GetThreadId();
	printf("MALLOC PERF TEST: Thread %i: Allocating %llu memory blocks of constant size %llu Bytes\n", Id, BlkCountLimit, Size0);

	for (TSize i = 0; i < BlkCountLimit; ++i)
	{
		Worker->GetTimer()->Start();
		//Ptr = LocalAlloc(LPTR, Size0);
		//Ptr = malloc(Size0);
		Ptr = Malloc(Size0);
		Worker->GetTimer()->Stop();
		Worker->MallocTimeStats.BlockAllocTime += Worker->GetTimer()->GetDuration();
		ShowProgress((float64)i / (float64)BlkCountLimit, 1.0f);
		if (!Ptr)
		{
			printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY Line: %i\n", __LINE__);
			//SafeShutdownMalloc();
			TWorker::ExitCode.store(EXIT_FAILURE);
			return;
		}
	}

	printf("MALLOC PERF TEST: CONST SIZE BLOCK ALLOCATION TEST  is completed\n");

	std::string Str{};
	Str += "--------------------- BLOCK ALLOCATION TEST ----------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Allocating of memory blocks of constant size:\n";
	Str += "Block size: " + std::to_string(Size0) + " Bytes\tBlock count: " + std::to_string(BlkCountLimit) + "\n";

	GLogger->DumpStrToFile(Str.c_str());
}

void Test_Perf_Malloc_Const_Blocks_2(TWorker* Worker)
{
	void* Ptr = nullptr;
	TSize Size0 = 65536;
	uint64 BlkCountLimit = 100000;
	uint32 Id = Worker->GetThreadId();
	printf("MALLOC PERF TEST: Thread %i: Allocating %llu memory blocks of constant size %llu Bytes\n", Id, BlkCountLimit, Size0);

	for (TSize i = 0; i < BlkCountLimit; ++i)
	{
		Worker->GetTimer()->Start();
		//Ptr = malloc(Size0);
		Ptr = Malloc(Size0);
		Worker->GetTimer()->Stop();
		Worker->MallocTimeStats.BlockAllocTime += Worker->GetTimer()->GetDuration();
		ShowProgress((float64)i / (float64)BlkCountLimit, 1.0f);
		if (!Ptr)
		{
			printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY\n");
			//SafeShutdownMalloc();
			TWorker::ExitCode.store(EXIT_FAILURE);
			return;
		}
	}

	printf("MALLOC PERF TEST: CONST SIZE BLOCK ALLOCATION TEST  is completed\n");

	std::string Str{};
	Str += "--------------------- BLOCK ALLOCATION TEST ----------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Allocating of memory blocks of constant size:\n";
	Str += "Block size: " + std::to_string(Size0) + " Bytes\tBlock count: " + std::to_string(BlkCountLimit) + "\n";

	GLogger->DumpStrToFile(Str.c_str());
}

void Test_Perf_Malloc_Progressive_Blocks(TWorker* Worker)
{
	void* Ptr = nullptr;
	TSize MaxPoolBlockSize = GetMaxPoolBlockSize();
	TSize SizeDelta = 16384;
	TSize Size0 = 8;
	uint64 k = 0;
	TSize SzPrev = 0;
	TSize SzSum = 0;
	TSize Sz = 0;
	uint32 Id = Worker->GetThreadId();
	printf("MALLOC PERF TEST: Thread %i: Allocating memory blocks of progressive size: Min size: %llu Bytes, Max size: %llu Bytes\n", Id, Size0, MaxPoolBlockSize);

	for (Sz = Size0; Sz < MaxPoolBlockSize; Sz += SizeDelta)
	{
		SzSum = SzPrev + Sz;

		//if (i == 100785)
		//{
		//	__nop();
		//	__nop();
		//  __debugbreak();
		//}

		if (SzSum > 8589934592)
		{
			break;
		}


		for (TSize i = 0; i < 4; ++i)
		{
			Worker->GetTimer()->Start();
			Ptr = Malloc(Sz);
			Worker->GetTimer()->Stop();
			Worker->MallocTimeStats.BlockAllocTime += Worker->GetTimer()->GetDuration();
/*
!!! It's very dengerous loop !!!! ;)
					  ||
					  \/
*/
			for (TSize j = 0; j < Sz; j += 2000)
			{
				*((uint8*)Ptr + j) = 0;
			}

			if (!Ptr)
			{
				printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY alloc number: %llu Sum allocated: %llu Bytes\n", i, SzSum);
				//SafeShutdownMalloc();
				TWorker::ExitCode.store(EXIT_FAILURE);
				return;
			}
			++k;
		}
		ShowProgress((float64)Sz / (float64)MaxPoolBlockSize, 1.0f);
		
		SzPrev = SzSum;
	}

	printf("MALLOC PERF TEST: PROGRESSIVE SIZE BLOCK ALLOCATION TEST is completed\n");

	std::string Str{};
	Str += "--------------------- BLOCK ALLOCATION TEST ----------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Allocating of memory blocks of progressive size:\n";
	Str += "Min size: " + std::to_string(Size0) + " Bytes\tMax size: " + std::to_string(MaxPoolBlockSize) + " Bytes\tBlock count: " + std::to_string(k) + "\n";

	GLogger->DumpStrToFile(Str.c_str());
}


void Test_Perf_Free(TWorker* Worker)
{
	void* Ptr[256] = { nullptr };
	TSize MaxPoolBlockSize = GetMaxPoolBlockSize();
	//MaxPoolBlockSize >>= 2;
	TSize SizeDelta = 16;
	TSize Size0 = 8;
	TSize SzSum = 0;
	TSize SzPrev = 0;
	uint32 Id = Worker->GetThreadId();
	printf("MALLOC PERF TEST: Thread %i: Releasing memory blocks of different sizes. Min block size: %llu Bytes; Max block size: %llu Bytes\n", Id, Size0, MaxPoolBlockSize);
	uint32 k = 0;
	for (TSize Sz = Size0; Sz < MaxPoolBlockSize; Sz += SizeDelta)
	//for (TSize j = Size0; j < 1000000; ++j)
	{
		SzSum = SzPrev + Sz;
		for (TSize i = 0; i < 4; ++i)
		{
			Ptr[i] = Malloc(Sz);
			//Ptr[i] = malloc(Sz);
			if (!Ptr[i])
			{
				printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY, ITERATION: %i, ALLOCATED: %llu\n", k, SzSum);
				//SafeShutdownMalloc();
				TWorker::ExitCode.store(EXIT_FAILURE);
				return;
			}
			++k;
		}

		for (TSize i = 0; i < 4; ++i)
		{
			Worker->GetTimer()->Start();
			//free(Ptr[i]);
			Free(Ptr[i]);
			Worker->GetTimer()->Stop();
			Worker->MallocTimeStats.BlockFreeTime += Worker->GetTimer()->GetDuration();
		}
		ShowProgress((float64)Sz / (float64)MaxPoolBlockSize, 1.0f);
		SzPrev = SzSum;
		//ShowProgress((float64)j / (float64)1000000, 1.0f);
	}

	printf("MALLOC PERF TEST: BLOCK RELEASING TEST is completed\n");

	std::string Str{};
	Str += "--------------------- BLOCK RELEASING TEST -----------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Releasing of memory blocks of variable size:\n";
	Str += "Min size: " + std::to_string(Size0) + " Bytes\tMax size: " + std::to_string(MaxPoolBlockSize) + " Bytes\tBlock count: " + std::to_string(k) + "\n";

	GLogger->DumpStrToFile(Str.c_str());
}

void Test_Perf_Small_Reallocs(TWorker* Worker)
{
	struct Blocks
	{
		TSize Size;
		void* Ptr;
	} B[256] = { 0, nullptr };

	TSize MaxPoolBlockSize = GetMaxPoolBlockSize();
	//MaxPoolBlockSize >>= 4;
	TSize SizeDelta = 1024;
	TSize Size0 = 8;
	uint32 Id = Worker->GetThreadId();
	printf("MALLOC PERF TEST: Thread %i:  memory blocks SMALL reallocations.\n", Id);
	uint32 k = 0;
	for (TSize Sz = Size0; Sz < MaxPoolBlockSize; Sz += SizeDelta)
	{
		for (TSize i = 0; i < 4; ++i)
		{
			B[i].Ptr = Malloc(Sz);
			//B[i].Ptr = malloc(Sz);
			B[i].Size = Sz;

			if (!B[i].Ptr)
			{
				printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY. Program is terminated\n");
				//SafeShutdownMalloc();
				TWorker::ExitCode.store(EXIT_FAILURE);
				return;
			}
			++k;
		}

		for (TSize i = 0; i < 4; ++i)
		{
			Worker->GetTimer()->Start();
			B[i].Ptr = Realloc(B[i].Ptr, B[i].Size + (B[i].Size >> 5));
			//B[i].Ptr = realloc(B[i].Ptr, B[i].Size + (B[i].Size >> 5));
			Worker->GetTimer()->Stop();
			Worker->MallocTimeStats.BlockReallocTime += Worker->GetTimer()->GetDuration();
		}

		for (TSize i = 0; i < 4; ++i)
		{
			Free(B[i].Ptr);
		}
		ShowProgress((float64)Sz / (float64)MaxPoolBlockSize, 1.0f);

	}

	printf("MALLOC PERF TEST: SMALL REALLOCATIONS TEST is completed\n");

	std::string Str{};
	Str += "------------------- SMALL REALLOCATIONS TEST ---------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Small reallocations of memory blocks of variable size:\n";
	Str += "Min size: " + std::to_string(Size0) + " Bytes\tMax size: " + std::to_string(MaxPoolBlockSize) + " Bytes\tBlock count: " + std::to_string(k) + "\n";


	GLogger->DumpStrToFile(Str.c_str());
}

void Test_Perf_Big_Reallocs(TWorker* Worker)
{
	struct Blocks
	{
		TSize Size;
		void* Ptr;
	} B[256] = { 0, nullptr };

	TSize MaxPoolBlockSize = GetMaxPoolBlockSize();
	TSize SizeDelta = 1024;
	TSize Size0 = 8;
	uint32 Id = Worker->GetThreadId();
	uint32 k = 0;

	printf("MALLOC PERF TEST: Thread %i: memory blocks BIG reallocations.\n", Id);

	for (TSize Sz = Size0; Sz < MaxPoolBlockSize; Sz += SizeDelta)
	{
		for (TSize i = 0; i < 4; ++i)
		{
			B[i].Ptr = Malloc(Sz);
			//B[i].Ptr = malloc(Sz);
			B[i].Size = Sz;

			if (!B[i].Ptr)
			{
				printf("\nMALLOC PERF TEST: PANIC!!! OUT OF MEMORY. Program is terminated\n");
				//SafeShutdownMalloc();
				TWorker::ExitCode.store(EXIT_FAILURE);
				return;
			}
			++k;
		}

		for (TSize i = 0; i < 4; ++i)
		{
			Worker->GetTimer()->Start();
			B[i].Ptr = Realloc(B[i].Ptr, B[i].Size * 4);
			//B[i].Ptr = realloc(B[i].Ptr, B[i].Size * 4);
			Worker->GetTimer()->Stop();
			Worker->MallocTimeStats.BlockReallocTime += Worker->GetTimer()->GetDuration();
		}

		for (TSize i = 0; i < 4; ++i)
		{
			Free(B[i].Ptr);
		}

		ShowProgress((float64)Sz / (float64)MaxPoolBlockSize, 1.0f);
	}

	printf("MALLOC PERF TEST: BIG REALLOCATIONS TEST is completed\n");

	std::string Str{};
	Str += "------------------- BIG REALLOCATIONS TEST ---------------------\n";
	Str += "MALLOC PERF TEST: " + std::string("Thread: ") + std::to_string(Id) + "\n";
	Str += "Big reallocations of memory blocks of variable size:\n";
	Str += "Min size: " + std::to_string(Size0) + " Bytes\tMax size: " + std::to_string(MaxPoolBlockSize) + " Bytes\tBlock count: " + std::to_string(k) + "\n";

	GLogger->DumpStrToFile(Str.c_str());
}
