#pragma once

#include "std.h"

template<typename T>
inline constexpr T AlignToUpper(T Value, TSize Alignment)
{
	return (T)(((uint64)Value + Alignment - 1) & ~(Alignment - 1));
}

//inline bool IsAligned(const void* Ptr, const TSize Alignment)
//{
//	return !((TSize)(Ptr) & (Alignment - 1));
//}

template<typename T>
inline constexpr T AlignToLower(T Value, TSize Alignment)
{
	return (T)(((uint64)Value) & ~(Alignment - 1));
}
//
//
//inline void* AlignPtrToUpper(void* Offset, TSize Alignment)
//{
//	return reinterpret_cast<void*>((reinterpret_cast<TSize>(Offset) + (Alignment - 1)) & ~(Alignment - 1));
//}

template<typename T>
inline constexpr bool IsAligned(T Value, const TSize Alignment)
{
	return !((uint64)Value & (Alignment - 1));
}

inline bool IsPartOf(const void* Ptr, const void* Start, TSize Size)
{
	const void* End = static_cast<const uint8_t*>(Start) + Size;
	return (Ptr < End && Ptr >= Start);
}