#pragma once

#include "std.h"
#include "defs.h"
#include "imalloc.h"
#include "malloc_stats.h"

static const TSize MALLOC_DEFAULT_ALIGNMENT = 16;

class TMallocBase :
	public IMalloc
{
public:

	virtual bool Init() = 0;
	virtual bool IsInitialized() = 0;
	virtual void Shutdown() = 0;
	virtual TSize GetMallocMaxAlignment() = 0;

	void GetMallocStats(TMallocStats& Stats)
	{
#ifdef MALLOC_STATS
		Stats = MallocStats;
#else
		// log;
#ifdef MALLOC_SCALED_DEBUG
		printf("Malloc stats are not enabled");
#endif // DEBUG
#endif
	}

	void GetMallocTimeStats(TMallocTimeStats& TimeStats)
	{
#ifdef MALLOC_TIME_STATS
		TimeStats = MallocTimeStats;
#else
		// log;
#ifdef MALLOC_SCALED_DEBUG
		printf("Malloc time stats are not enabled");
#endif
#endif

	}

	virtual void GetSpecificStats(void* OutStatData) = 0;

protected:
#ifdef MALLOC_STATS
	TMallocStats MallocStats;
#endif
#ifdef	MALLOC_TIME_STATS
	TMallocTimeStats MallocTimeStats;
#endif
};
