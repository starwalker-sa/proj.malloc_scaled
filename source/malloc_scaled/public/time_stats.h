#pragma once


#include "std.h"

class TTimeStats
{
public:
	TTimeStats() :
		MaxTime(0),
		MinTime(TDuration::max()),
		AvgTime(0),
		LastTime(0),
		TotalTime(0),
		MeasureCount(0)
	{

	}

	void Reset()
	{
		MaxTime = {};
		MinTime = TDuration::max();
		AvgTime = {};
		LastTime = {};
		TotalTime = {};

		MeasureCount = 0;
	}

	void AddInterval(TDuration TimeInterval)
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

	TTimeStats& operator+(TDuration TimeInterval)
	{
		AddInterval(TimeInterval);
		return *this;
	}

	TTimeStats& operator+=(TDuration TimeInterval)
	{
		return *this + TimeInterval;
	}

	TDuration GetMaxTime()
	{
		return MaxTime;
	}

	TDuration GetMinTime()
	{
		return MinTime;
	}

	TDuration GetAvgTime()
	{
		return AvgTime;
	}

	TDuration GetLastTime()
	{
		return LastTime;
	}

	TDuration GetTotalTime()
	{
		return TotalTime;
	}

	uint64    GetMeasureCount()
	{
		return MeasureCount;
	}

private:

	TDuration MaxTime;
	TDuration MinTime;
	TDuration AvgTime;
	TDuration LastTime;
	TDuration TotalTime;

	uint64 MeasureCount;
};