
#include "malloc_stats.h"

void TTimeStats::AddInterval(TDuration TimeInterval)
{
	LastTime = TimeInterval;
	++MeasureCount;

	if (TimeInterval > MaxTime)
		MaxTime = TimeInterval;

	if (TimeInterval < MinTime)
		MinTime = TimeInterval;

	TotalTime += TimeInterval;
	AvgTime = TotalTime / MeasureCount;
}

TTimeStats& TTimeStats::operator+(TDuration TimeInterval)
{
	AddInterval(TimeInterval);
	return *this;
}

TTimeStats& TTimeStats::operator+=(TDuration TimeInterval)
{
	return *this + TimeInterval;
}

TDuration TTimeStats::GetMaxTime()
{
	return MaxTime;
}

TDuration TTimeStats::GetMinTime()
{
	return MinTime;
}

TDuration TTimeStats::GetAvgTime()
{
	return AvgTime;
}

TDuration TTimeStats::GetLastTime()
{
	return LastTime;
}

TDuration TTimeStats::GetTotalTime()
{
	return TotalTime;
}

uint64    TTimeStats::GetMeasureCount()
{
	return MeasureCount;
}

void TTimeStats::Reset()
{
	MaxTime = {};
	MinTime = TDuration::max();
	AvgTime = {};
	LastTime = {};
	TotalTime = {};

	MeasureCount = 0;
}