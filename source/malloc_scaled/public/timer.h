#pragma once

#include "std.h"

class TTimer
{
public:
	TTimer();
	void Start();
	void Stop();
	TDuration GetDuration();

private:
	std::chrono::high_resolution_clock::time_point StartTick;
	std::chrono::high_resolution_clock::time_point EndTick;
};
