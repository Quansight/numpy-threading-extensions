#pragma once

#include <cstdlib>
#include <limits>
#include <stdint.h>

#if defined(_WIN32) && !defined(__GNUC__)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
// Windows Header Files:
#include <Windows.h>
#include <winnt.h>
#endif


/*
Macro symbol definitions to simplify conditional code compilation within riptide.

References:
* https://sourceforge.net/p/predef/wiki/Compilers/

*/

/*
Platform/OS detection
*/

#if defined(_WIN32)
// Target OS is Windows
#   define RT_OS_WINDOWS 1

#elif defined(__linux__)
// Target OS is Linux
#   define RT_OS_LINUX 1

    // Target OS is UNIX-like
#   define RT_OS_FAMILY_UNIX 1

#elif defined(__APPLE__)
// Target OS is macOS or iOS
#   define RT_OS_DARWIN 1

    // Target OS is UNIX-like
#   define RT_OS_FAMILY_UNIX 1

    // Target OS is BSD-like
#   define RT_OS_FAMILY_BSD 1

#elif __FreeBSD__
// Target OS is FreeBSD
#   define RT_OS_FREEBSD 1

    // Target OS is UNIX-like
#   define RT_OS_FAMILY_UNIX 1

    // Target OS is BSD-like
#   define RT_OS_FAMILY_BSD 1

#else
// If we can't detect the OS, make it a compiler error; compilation is likely to fail anyway due to
// not having any working implementations of some functions, so at least we can make it obvious why
// the compilation is failing.
#   error Unable to detect/classify the target OS.

#endif  /* Platform/OS detection */


/*
Compiler detection.
The order these detection checks operate in is IMPORTANT -- use CAUTION if changing or reordering them!
*/

#if defined(__clang__)
// Compiler is Clang/LLVM.
#   define RT_COMPILER_CLANG 1

#elif defined(__GNUC__)
// Compiler is GCC/g++.
#   define RT_COMPILER_GCC 1

#elif defined(__INTEL_COMPILER) || defined(_ICC)
// Compiler is the Intel C/C++ compiler.
#   define RT_COMPILER_INTEL 1

#elif defined(_MSC_VER)
/*
This check needs to be towards the end; a number of compilers (e.g. clang, Intel C/C++)
define the _MSC_VER symbol when running on Windows, so putting this check last means we
should have caught any of those already and this should be bona-fide MSVC.
*/
// Compiler is the Microsoft C/C++ compiler.
#   define RT_COMPILER_MSVC 1

#else
// Couldn't detect the compiler.
// We could allow compilation to proceed anyway, but the compiler/platform behavior detection
// below won't pass and it's important for correctness so this is an error.
#   error Unable to detect/classify the compiler being used.

#endif  /* compiler detection */


/*
Compiler behavior detection.
For conciseness/correctness in riptide code, we define some additional symbols here specifying certain
compiler behaviors. This way any code depending on these behaviors expresses it in terms of the behavior
rather than whether it's being compiled under a specific compiler(s) and/or platforms; this in turn
makes it easier to support new compilers and platforms just by adding the necessary defines here.
*/

#if !defined(RT_COMPILER_MSVC)
// Indicates whether the targeted compiler/platform defaults to emitting vector load/store operations
// requiring an aligned pointer when a vector pointer is dereferenced (so any such pointers must be
// aligned to prevent segfaults). When zero/false, the targeted compiler/platform emits unaligned
// vector load/store instructions by default.
#   define RT_TARGET_VECTOR_MEMOP_DEFAULT_ALIGNED 1
#else
// Indicates whether the targeted compiler/platform defaults to emitting vector load/store operations
// requiring an aligned pointer when a vector pointer is dereferenced (so any such pointers must be
// aligned to prevent segfaults). When zero/false, the targeted compiler/platform emits unaligned
// vector load/store instructions by default.
#   define RT_TARGET_VECTOR_MEMOP_DEFAULT_ALIGNED 0
#endif  /* RT_TARGET_VECTOR_MEMOP_DEFAULT_ALIGNED */

//-------------------------------------------
typedef void* HANDLE;
#define TRUE 1
#define FALSE 0
typedef int BOOL;
typedef unsigned char       BYTE;


#if defined(_WIN32) && !defined(__GNUC__)
#define WINAPI      __stdcall
#define InterlockedCompareExchange128 _InterlockedCompareExchange128
#ifndef InterlockedAdd64
#define InterlockedAdd64 _InterlockedAdd64
#endif
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedIncrement64 _InterlockedIncrement64

#define YieldProcessor _mm_pause
#define InterlockedIncrement _InterlockedIncrement

#define FMInterlockedOr(X,Y) _InterlockedOr64((int64_t*)X,Y)

#include <intrin.h>
#ifndef MEM_ALIGN
#define MEM_ALIGN(x) __declspec(align(x))
#define ALIGN(x) __declspec(align(64))

#define FORCEINLINE __forceinline
#define FORCE_INLINE __forceinline

#define ALIGNED_ALLOC(Size,Alignment) _aligned_malloc(Size,Alignment)
#define ALIGNED_FREE(block) _aligned_free(block)

#define lzcnt_64 _lzcnt_u64

#endif
#else

#define WINAPI
#include <pthread.h>

// consider sync_add_and_fetch
#define InterlockedAdd64(val, len) (__sync_fetch_and_add(val, len) + len)
#define InterlockedIncrement64(val) (__sync_fetch_and_add(val, 1) + 1)
#define YieldProcessor _mm_pause
#define InterlockedIncrement(val) (__sync_fetch_and_add(val, 1) + 1)
#define FMInterlockedOr(val, bitpos) (__sync_fetch_and_or(val, bitpos))


#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(major, minor) ((((__GNUC__) << 16) + (__GNUC_MINOR__)) >= (((major) << 16) + (minor)))
#endif
#if __GNUC_PREREQ(4, 4) || (__clang__ > 0 && __clang_major__ >= 3) || !defined(__GNUC__)
/* GCC >= 4.4 or clang or non-GCC compilers */
#include <x86intrin.h>
#elif __GNUC_PREREQ(4, 1)
/* GCC 4.1, 4.2, and 4.3 do not have x86intrin.h, directly include SSE2 header */
#include <emmintrin.h>
#endif
#ifndef MEM_ALIGN
#define MEM_ALIGN(x) __attribute__((aligned(x)))
#endif

#define FORCEINLINE inline __attribute__((always_inline))
#define FORCE_INLINE inline __attribute__((always_inline))
#define ALIGN(x) x __attribute__((aligned(64)))

// Workaround for platforms/compilers which don't support C11 aligned_alloc
// but which do have posix_memalign().
#ifndef aligned_alloc

#ifdef posix_memalign
FORCEINLINE void* aligned_alloc(size_t alignment, size_t size)
{
    void* buffer = NULL;
    posix_memalign(&buffer, alignment, size);
    return buffer;
}

#else
// clang compiler does not support so we default to malloc
//#warning Unable to determine how to perform aligned allocations on this platform.
#define aligned_alloc(alignment, size) malloc(size)
#endif  // defined(posix_memalign)

#endif  // !defined(aligned_alloc)

#define ALIGNED_ALLOC(Size,Alignment) aligned_alloc(Alignment,Size)
#define ALIGNED_FREE(block) free(block)

#define lzcnt_64 __builtin_clzll

#endif

enum ATOP_TYPES {
    ATOP_BOOL = 0,
    ATOP_INT8, ATOP_UINT8,
    ATOP_INT16, ATOP_UINT16,
    ATOP_INT32, ATOP_UINT32,
    ATOP_INT64, ATOP_UINT64,
    ATOP_INT128, ATOP_UINT128,
    ATOP_FLOAT, ATOP_DOUBLE, ATOP_LONGDOUBLE,
    ATOP_STRING, ATOP_UNICODE,
    ATOP_VOID,
    ATOP_LAST
};


enum MATH_OPERATION {
    // Two ops, returns same type
    ADD = 1,
    SUB = 2,
    MUL = 3,
    MOD = 4,
    MIN = 5,
    MAX = 6,
    NANMIN = 7,
    NANMAX = 8,
    FLOORDIV = 9,
    POWER = 10,
    REMAINDER = 11,
    FMOD = 12,

    // Two ops, always return a double
    DIV = 13,
    SUBDATETIMES = 14,  // returns double
    SUBDATES = 15,   // returns int

    // One input, returns same data type
    ABS = 17,
    NEG = 18,
    FABS = 19,
    INVERT = 20,
    FLOOR = 21,
    CEIL = 22,
    TRUNC = 23,
    ROUND = 24,
    NEGATIVE = 25,
    POSITIVE = 26,
    SIGN = 27,
    RINT = 28,
    EXP = 29,
    EXP2 = 30,

    // One input, always return a float one input
    SQRT = 31,
    LOG = 32,
    LOG2 = 33,
    LOG10 = 34,
    EXPM1 = 35,
    LOG1P = 36,
    SQUARE = 37,
    CBRT = 38,
    RECIPROCAL = 39,

    // Two inputs, Always return a bool
    CMP_EQ = 41,
    CMP_NE = 42,
    CMP_LT = 43,
    CMP_GT = 44,
    CMP_LTE = 45,
    CMP_GTE = 46,
    LOGICAL_AND = 47,
    LOGICAL_XOR = 48,
    LOGICAL_OR = 49,

    // Two inputs, second input must be int based
    BITWISE_LSHIFT = 51,
    BITWISE_RSHIFT = 52,
    BITWISE_AND = 53,
    BITWISE_XOR = 54,
    BITWISE_OR = 55,
    BITWISE_ANDNOT = 56,
    BITWISE_NOTAND = 57,
    BITWISE_XOR_SPECIAL = 58,

    // one input, output bool
    LOGICAL_NOT = 61,
    ISINF = 62,
    ISNAN = 63,
    ISFINITE = 64,
    ISNORMAL = 65,

    ISNOTINF = 66,
    ISNOTNAN = 67,
    ISNOTFINITE = 68,
    ISNOTNORMAL = 69,
    ISNANORZERO = 70,
    SIGNBIT = 71,

    // One input, does not allow floats
    BITWISE_NOT = 72,

    LAST = 73,
};


static int64_t  gDefaultInt64 = 0x8000000000000000;
static int32_t  gDefaultInt32 = 0x80000000;
static uint16_t  gDefaultInt16 = 0x8000;
static uint8_t   gDefaultInt8 = 0x80;

static uint64_t gDefaultUInt64 = 0xFFFFFFFFFFFFFFFF;
static uint32_t gDefaultUInt32 = 0xFFFFFFFF;
static uint16_t gDefaultUInt16 = 0xFFFF;
static uint8_t  gDefaultUInt8 = 0xFF;

static float  gDefaultFloat = std::numeric_limits<float>::quiet_NaN();
static double gDefaultDouble = std::numeric_limits<double>::quiet_NaN();
static long double gDefaultLongDouble = std::numeric_limits<long double>::quiet_NaN();
static int8_t gDefaultBool = 0;
static char   gString[1024] = { 0,0,0,0 };

//----------------------------------------------------
// returns pointer to a data type (of same size in memory) that holds the invalid value for the type
// does not yet handle strings
static void* GetDefaultForType(int numpyInType) {
    void* pgDefault = &gDefaultInt64;

    switch (numpyInType) {
    case ATOP_FLOAT:  pgDefault = &gDefaultFloat;
        break;
    case ATOP_DOUBLE: pgDefault = &gDefaultDouble;
        break;
    case ATOP_LONGDOUBLE: pgDefault = &gDefaultLongDouble;
        break;
    // BOOL should not really have an invalid value inhabiting the type
    case ATOP_BOOL:   pgDefault = &gDefaultBool;
        break;
    case ATOP_INT8:   pgDefault = &gDefaultInt8;
        break;
    case ATOP_INT16:  pgDefault = &gDefaultInt16;
        break;
    case ATOP_INT32:  pgDefault = &gDefaultInt32;
        break;
    case ATOP_INT64:  pgDefault = &gDefaultInt64;
        break;
    case ATOP_UINT8:  pgDefault = &gDefaultUInt8;
        break;
    case ATOP_UINT16: pgDefault = &gDefaultUInt16;
        break;
    case ATOP_UINT32: pgDefault = &gDefaultUInt32;
        break;
    case ATOP_UINT64: pgDefault = &gDefaultUInt64;
        break;
    case ATOP_STRING: pgDefault = &gString;
        break;
    case ATOP_UNICODE: pgDefault = &gString;
        break;
    default:
        printf("!!! likely problem in GetDefaultForType\n");
    }

    return pgDefault;
}

static const int GetMaoType(bool value) {
    return ATOP_BOOL;
}
static const int GetMaoType(int8_t value) {
    return ATOP_INT8;
}
static const int GetMaoType(int16_t value) {
    return ATOP_INT16;
}
static const int GetMaoType(int32_t value) {
    return ATOP_INT32;
}
static const int GetMaoType(int64_t value) {
    return ATOP_INT64;
}
static const int GetMaoType(uint8_t value) {
    return ATOP_UINT8;
}
static const int GetMaoType(uint16_t value) {
    return ATOP_UINT16;
}
static const int GetMaoType(uint32_t value) {
    return ATOP_UINT32;
}

static const int GetMaoType(uint64_t value) {
    return ATOP_UINT64;
}
static const int GetMaoType(float value) {
    return ATOP_FLOAT;
}
static const int GetMaoType(double value) {
    return ATOP_DOUBLE;
}
static const int GetMaoType(long double value) {
    return ATOP_LONGDOUBLE;
}
static const int GetMaoType(char* value) {
    return ATOP_STRING;
}


template <typename T>
static void* GetInvalid() {
    return GetDefaultForType(GetMaoType((T)0));
}

//----------------------------------------------------------------------------------
// Lookup to go from 1 byte to 8 byte boolean values
extern int64_t gBooleanLUT64[256];
extern int32_t gBooleanLUT32[16];

extern int64_t gBooleanLUT64Inverse[256];
extern int32_t gBooleanLUT32Inverse[16];


//-----------------------------------------------------------
// Build a list of callable vector functions
enum TYPE_OF_FUNCTION_CALL {
    ANY_ONE = 1,
    ANY_TWO = 2,
    ANY_THREEE = 3,
    ANY_GROUPBY_FUNC = 4,
    ANY_GROUPBY_XFUNC32 = 5,
    ANY_GROUPBY_XFUNC64 = 6,
    ANY_SCATTER_GATHER = 7,
    ANY_MERGE_TWO_FUNC = 8,
    ANY_MERGE_STEP_ONE = 9
};

enum SCALAR_MODE {
    NO_SCALARS = 0,
    FIRST_ARG_SCALAR = 1,
    SECOND_ARG_SCALAR = 2,
    BOTH_SCALAR = 3   // not used
};

struct stScatterGatherFunc {
    // numpy intput ttype
    int32_t inputType;

    // the core (if any) making this calculation
    int32_t core;

    // used for nans, how many non nan values
    int64_t lenOut;

    // !!must be set when used by var and std
    double meanCalculation;

    double resultOut;

    // Separate output for min/max
    int64_t  resultOutInt64;

};

typedef double(*ANY_SCATTER_GATHER_FUNC)(void* pDataIn, int64_t len, stScatterGatherFunc* pstScatterGatherFunc);
typedef void(*UNARY_FUNC)(void* pDataIn, void* pDataOut, int64_t len, int64_t strideIn, int64_t strideOut);
typedef void(*UNARY_FUNC_STRIDED)(void* pDataIn, void* pDataOut, int64_t len, int64_t strideIn, int64_t strideOut);
// Pass in two vectors and return one vector
// Used for operations like C = A + B
typedef void(*ANY_TWO_FUNC)(void* pDataIn, void* pDataIn2, void* pDataOut, int64_t len, int32_t scalarMode);
typedef void(*GROUPBY_FUNC)(void* pstGroupBy, int64_t index);

//-----------------------------------------------------------
// List of function calls
struct FUNCTION_LIST {
    int16_t             TypeOfFunctionCall; // See enum
    int16_t             NumpyType;         // For the array and constants
    int16_t             NumpyOutputType;

    // The item size for two input arrays assumed to be the same
    int64_t             InputItemSize;
    int64_t             OutputItemSize;

    // Strides may be 0 if it is a scalar or length 1
    int64_t             Input1Strides;
    int64_t             Input2Strides;

    // TODO: Why not make this void and recast?
    // Only one of these can be set
    union {
        void* FunctionPtr;
        ANY_SCATTER_GATHER_FUNC     AnyScatterGatherCall;
        UNARY_FUNC        AnyOneStubCall;
        ANY_TWO_FUNC      AnyTwoStubCall;
        GROUPBY_FUNC      GroupByCall;
    };

    const char* FunctionName;
};


//======================================================
// Unary
//------------------------------------------------------
// Macro stub for returning None
#define STRIDE_NEXT(_TYPE_, _MEM_, _STRIDE_) (_TYPE_*)((char*)_MEM_ + _STRIDE_)


//--------------------------------------------------------------------
// multithreaded struct used for calling unary op codes
struct UNARY_CALLBACK {
    union {
        UNARY_FUNC pUnaryCallback;
        UNARY_FUNC_STRIDED pUnaryCallbackStrided;
    };

    char* pDataIn;
    char* pDataOut;

    int64_t itemSizeIn;
    int64_t itemSizeOut;
};

//====================================================================
void* FmAlloc(size_t _Size);
void FmFree(void* _Block);

#define WORKSPACE_ALLOC FmAlloc
#define WORKSPACE_FREE FmFree