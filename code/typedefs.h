#include <stdint.h>

#if !defined TYPEDEFS_H

#if !defined (COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#define local_persist static
#define global_variable static
#define internal static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define pi32 3.14159265359f

#if TEST_SLOW
#define Assert(expression) if (!(expression)) *(int*)0 = 0;
#else
#define Assert(expression)
#endif


#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define Minimum(a, b) ((a < b) ? (a) : (b))
#define Maximum(a, b) ((a > b) ? (a) : (b))


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i32 bool32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

#define TYPEDEFS_H
#endif
