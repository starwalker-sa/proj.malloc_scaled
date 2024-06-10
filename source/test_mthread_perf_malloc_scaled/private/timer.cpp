#include "timer.h"


TTimer::TTimer()
{
	Start();
}

void TTimer::Start()
{
	StartTick = std::chrono::high_resolution_clock::now();
}

void TTimer::Stop()
{
	EndTick = std::chrono::high_resolution_clock::now();
}

TDuration TTimer::GetDuration()
{
	return TDuration(EndTick - StartTick);
}


