#include "test_mthread_perf_malloc.h"
#include "timer.h"
#include <vector>
#include <memory>
#include <cstring>
#include <string>

using std::unique_ptr;
using std::vector;
vector<unique_ptr<TWorker>>* Workers = nullptr;

int32 ParseCmdLine(const char* Line)
{
	const char* NumStr = nullptr;
	const char* Token = "--thread-count=";
	TSize TokenLen = strlen("--thread-count=");

	for (int32 i = 0; i < strlen(Line); ++i)
	{
		if (Line[i] != Token[i])
		{
			return 0;
		}

		if (Token[i] == '=' && i == (TokenLen - 1))
		{
			NumStr = &Line[TokenLen];
			break;
		}
	}

	if (NumStr == 0)
	{
		return 0;
	}
	
	int32 Num = std::stoi(NumStr);
	return Num;
}

int main(int Argc, char* Argv[])
{
	std::string ExePath{ Argv[0] };
	std::string Path = ExePath.substr(0, ExePath.find_last_of("\\/")) + "\\multi_thread_perf_tests.txt";

	int32 NumOfThreads = DEFAULT_MAX_CONCURENT_THREADS;
	if (Argc == 2)
	{
		int32 N = ParseCmdLine(Argv[1]);
		
		if (N)
		{
			NumOfThreads = N;
		}
		else
		{
			printf("MALLOC PERF TEST: Warning: Invalid first argument. Use: --thread-count='Count'\n");
			printf("MALLOC PERF TEST: Default thread count will be used\n");
		}
	}

	printf("MALLOC PERF TEST: Memory allocator performance tests\n");
	printf("MALLOC PERF TEST: Every task needs a while time to complete (several minutes), please wait!\n");
	printf("MALLOC PERF TEST: Number of threads will be created is %i\n", NumOfThreads);


	TLogger Logger(Path.c_str());
	GLogger = &Logger;

	Workers = new vector<unique_ptr<TWorker>>{};

	if (!Workers)
	{
		printf("MALLOC PERF TEST: Cannot init vector. Exit!\n");
		exit(EXIT_FAILURE);
	}

	GLogger->DumpStrToFile("=============== Memory allocator performance tests ===============\n");

	TTimer Timer;
	Timer.Start();

	for (int32 i = 0; i < NumOfThreads; ++i)
	{
		unique_ptr<TWorker> wp{ new TWorker() };
		if (!wp)
		{
			printf("MALLOC PERF TEST: Cannot init task thread. Exit!\n");
			exit(EXIT_FAILURE);
		}

		Workers->push_back(std::move(wp));
	}

	for (int32 i = 0; i < NumOfThreads; ++i)
	{
		(*Workers)[i]->Join();
	}

	Timer.Stop();
	int32 ExitCode = TWorker::ExitCode.load();

	if (ExitCode == 0)
	{
		printf("MALLOC PERF TEST: Tests was performed by %i threads\n", NumOfThreads);

		float64 TotTime = std::chrono::duration<float64, std::nano>(Timer.GetDuration()).count();

		printf("MALLOC PERF TEST: Total time of execution: %f seconds\n", TotTime / 1000000000.0f);

		std::string EndStr{};
		EndStr += "Total time of execution :" + std::to_string(TotTime / 1000000000.0f) + " seconds\n";
		EndStr += "============================== END ===============================\n";

		GLogger->DumpStrToFile(EndStr.c_str());

		printf("MALLOC PERF TEST: All stats data are written into the \"multi_threaded_perf_tests.txt\" file\n");
		printf("MALLOC PERF TEST: Buy!\n");
	}
	else
	{
		printf("MALLOC PERF TEST: Something went wrong inside testing thread executions... Exit\n");
	}

	printf("MALLOC PERF TEST: Press ENTER key to exit\n");
	auto ch = getchar();

	return ExitCode;
}