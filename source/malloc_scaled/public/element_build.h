#pragma once

#include "std.h"

template<typename TELEM>
void CreateElementsDefault(TELEM* Dst, TSize Count)
{
	while (Count)
	{
		new (Dst) TELEM{};
		++Dst;
		--Count;
	}
}

template<typename TELEM, typename... TARGS>
void CreateElementsFromArgs(TELEM* Dst, TSize Count, TARGS&&... Args)
{
	while (Count)
	{
		//new (Dst) TELEM(forward<TARGS>(Args)...);
		++Dst;
		--Count;
	}
}

template<typename TSRC, typename TDST>
void CreateElementsCopy(const TSRC* Src, TDST* Dst, TSize Count)
{
	while (Count)
	{
		new (Dst) TDST(*Src);
		++Src;
		++Dst;
		--Count;
	}
}

template<typename TELEM, typename TVAL>
void CreateElementsCopy(const TVAL& Src, TELEM* Dst, TSize Count)
{
	while (Count)
	{
		new(Dst) TELEM(Src);
		++Dst;
		--Count;
	}
}

template<typename TSRC, typename TDST>
void CreateElementsMove(TSRC* Src, TDST* Dst, TSize Count)
{
	while (Count)
	{
		new(Dst) TDST(move(*Src));
		++Src;
		++Dst;
		--Count;
	}
}

template<typename TELEM, typename TVAL>
void CreateElementsMove(TVAL&& Src, TELEM* Dst, TSize Count)
{
	while (Count)
	{
		new(Dst) TELEM(move(Src));
		++Dst;
		--Count;
	}
}

template<typename TELEM>
void DestroyElements(TELEM* Dst, TSize Count)
{
	while (Count)
	{
		Dst->~TELEM();
		++Dst;
		--Count;
	}
}

template<typename TELEM>
void CopyElements(const TELEM& Src, TELEM* Dst, TSize Count)
{
	while (Count)
	{
		*Dst = Src;
		++Dst;
		--Count;
	}
}

template<typename TELEM>
void CopyElements(const TELEM* Src, TELEM* Dst, TSize Count)
{
	while (Count)
	{
		*Dst = *Src;
		++Dst;
		++Src;
		--Count;
	}
}

template<typename TELEM>
void MoveElements(TELEM&& Src, TELEM* Dst, TSize Count)
{
	while (Count)
	{
		*Dst = move(Src);
		++Dst;
		--Count;
	}
}

template<typename TELEM>
void MoveElements(TELEM* Src, TELEM* Dst, TSize Count)
{
	TELEM* DstEnd = Dst + Count;
	TELEM* SrcEnd = Src + Count;

	if (Src < Dst && Dst < SrcEnd)
	{
		--DstEnd;
		--SrcEnd;
		//--Count;
		while (Count) 
		{ 
			*DstEnd-- = move(*SrcEnd--);
			--Count;
		}
	}

	else 
		while (Count) 
		{
			*Dst++ = move(*Src++);
			--Count; 
		}
}

