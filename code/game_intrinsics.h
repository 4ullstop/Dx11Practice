#if !defined(GAME_INTRINSICS_H)
#include "math.h"

inline i32
SignOf(i32 value)
{
    i32 result = (value >= 0) ? 1 : -1;
    return(result);
}

inline r32
SquareRoot(r32 real)
{
    r32 result = sqrtf(real);

    return(result);
}

inline r32
AbsoluteValue(r32 real)
{
    r32 result = (r32)fabs(real);
    return(result);
}

inline i32
RoundR32ToI32(r32 r32)
{
    i32 result = (i32)roundf(r32);
    return(result);
}

inline u32
RoundR32ToU32(r32 r32)
{
    u32 result = (u32)roundf(r32);
    return(result);
}


inline i32
FloorR32ToI32(r32 r32)
{
    i32 result = (i32)floorf(r32);
    return(result);
}

inline i32
CeilR32ToI32(r32 real)
{
    i32 result = (i32)ceilf(real);
    return(result);
}

inline i32
TruncateR32ToI32(r32 r32)
{
    i32 result = (i32)r32;
    return(result);
}

inline r32
Sin(r32 angle)
{
    r32 result = sinf(angle);
    return(result);
}

inline r32
Cos(r32 angle)
{
    r32 result = cosf(angle);
    return(result);
}

inline r32
ATan2(r32 y, r32 x)
{
    r32 result = atan2f(y, x);
    return(result);
}


struct bit_scan_result
{
    bool32 found;
    u32 index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 value)
{
    bit_scan_result result = {};
    result.found = false;

#if COMPILER_MSVC
    result.found = _BitScanForward((unsigned long*)&result.index, value);
#else
    for (u32 test = 0; test < 32; ++test)
    {
	//once we find the one thats set we breakout
	if(value & (1 << test))
	{
	    result.index = test;
	    result.found = true;
	    break;
	}
    }
#endif    
    return(result);
}

#define GAME_INTRINSICS_H
#endif
