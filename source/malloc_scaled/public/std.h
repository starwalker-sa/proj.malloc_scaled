
#pragma once

#include <cstdint>
#include <utility>
#include <cstring>
#include <cmath>
#include <limits>
#include <chrono>
#include <mutex>
//#include <algorithm>
#include <memory>
#include "defs.h"
#include <cstring>

using std::move;
using std::swap;
using std::find_if;
using std::forward;
using std::memcpy;
using std::mutex;
using std::strcmp;

#define MAX_ALIGN 16

typedef unsigned char		uint8;
typedef signed char			int8;
typedef unsigned short		uint16;
typedef signed short  		int16;
typedef unsigned int		uint32;
typedef signed int  		int32;
typedef unsigned long long	uint64;
typedef signed long long	int64;

typedef char	TChar;
typedef wchar_t	TWChar;
typedef uint8	T8Char;
typedef uint16	T16Char;
typedef uint32	T32Char;
typedef size_t  TSize;

typedef float  float32;
typedef double float64;

typedef std::chrono::duration<std::chrono::high_resolution_clock::rep, std::nano> TDuration;

#define KiB 1024
#define MiB 1024*1024
#define GiB 1024*1024*1024


inline constexpr uint64 FloorLog2(uint64 x)
{
	return x == 1 ? 0 : 1 + FloorLog2(x >> 1);
}

inline constexpr uint64 CeilLog2(uint64 x)
{
	return x == 1 ? 0 : FloorLog2(x - 1) + 1;
}

template<class T>
inline constexpr bool IsPow2(T Size)
{
	return (Size & (Size - 1)) == 0;
}

const uint64 tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5 };

inline constexpr uint64 Log2_64(uint64 value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((uint64)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}

uint64 constexpr Pow2_64(uint64 x)
{
    return x == 0 ? 1 : Pow2_64(x - 1) << 1;
}
