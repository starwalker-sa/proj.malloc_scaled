#pragma once
#include "lib_malloc.h"
#include <thread>
//#define _CRT_SECURE_NO_WARNINGS
#include <functional>
#include "timer.h"
#include <atomic>
#include <vector>
#include <memory>

using std::unique_ptr;
using std::vector;



#define DEFAULT_MAX_CONCURENT_THREADS 5

enum ETestType
{
	TEST_NONE,
	TEST_MALLOC,
	TEST_REALLOC,
	TEST_FREE
};

class TLogger;
class TWorker
{
public:
	TWorker():
		ThreadId(0),
		Worker(&TWorker::Run, this)
	{

	}

	void Join()
	{
		Worker.join();
	}

	//static int32 GetExitCode()
	//{
	//	return ExitCode.load();
	//}

	TTimer* GetTimer()
	{
		return &WorkerTimer;
	}

	uint32 GetThreadId()
	{
		return ThreadId;
	}

	void ResetStats()
	{
		MallocTimeStats.BlockAllocTime.Reset();
		MallocTimeStats.BlockReallocTime.Reset();
		MallocTimeStats.BlockFreeTime.Reset();
	}

	static ETestType TestType;
	TMallocTimeStats MallocTimeStats;
	static std::atomic<int32> ExitCode;

private:
	void Run();
	TTimer WorkerTimer;
	std::thread Worker;
	uint32 ThreadId;

	struct TTest
	{
		ETestType TestType;
		std::function<void(TWorker*)> TestFunction;
	};


	static const uint32 TestCount = 5;
	static TTest Tests[TestCount];
	static std::atomic<uint32> RunningTasks;
	static std::atomic<uint32> AllTasksCompleted;
};

class TLogger
{
public:

	TLogger(const char* FileName);
	~TLogger();

	void DumpStrToFile(const char* Str);

	void DumpStatsToFile(
		const char* Header1,
		ETestType TestType, uint32 TestNumber,
		uint64 MesCount, float64 MinTime, float64 MaxTime, float64 AvgTime);

	//static TLogger* GetLogger();
	//static bool Init();
	//static void Close();

private:


	//static TLogger* GLogger;

	mutex Guard;
	FILE* Log;
};

extern vector<unique_ptr<TWorker>>* Workers;
extern TLogger* GLogger;

void Test_Perf_Malloc_Const_Blocks_1(TWorker*);
void Test_Perf_Malloc_Const_Blocks_2(TWorker*);
void Test_Perf_Malloc_Progressive_Blocks(TWorker*);
void Test_Perf_Free(TWorker*);
void Test_Perf_Small_Reallocs(TWorker*);
void Test_Perf_Big_Reallocs(TWorker*);