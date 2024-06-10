#pragma once


template<typename TITER>
void SearchMaxMin(TITER First, TITER Last, TITER& Max, TITER& Min)
{
	Min = First;
	Max = First;
	TITER It = First;
	while (true)
	{
		if (*It < *Min)
		{
			Min = It;
		}
		else if (*It > *Max)
		{
			Max = It;
		}

		if (It == Last)
			break;
		++It;
	}

}