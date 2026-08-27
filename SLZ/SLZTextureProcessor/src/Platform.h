#ifndef SLZ_PLATFORM_HEADER
#define SLZ_PLATFORM_HEADER

#include "DllExport.h"
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__MINGW64)
	#define VK_USE_PLATFORM_WIN32_KHR
	#define PLATFORM_WINDOWS
	
	// Windows Header Files
	#define _CRT_SECURE_NO_DEPRECATE // Allow using portable fprintf instead of windows's fprintf_s	
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <fileapi.h>
	#define PATH_CHAR wchar_t
	#define PATH_LITERAL(val) L##val
	#define PATH_SEPARATOR L'\\'
	#define FILE_OPEN(file, path, mode) file = _wfsopen(path, mode, 0x40)
	#define PATH_PRINT(...) wprintf(__VA_ARGS__)
	#define PATH_SPRINT(...) _swprintf(__VA_ARGS__)

#elif defined(__linux__)
	#define PLATFORM_LINUX
	// linux header files
	#include <dlfcn.h>
	#include <string.h>
	#define PATH_CHAR char
	#define PATH_LITERAL(val) val
	#define PATH_SEPARATOR '/'
	#define FILE_OPEN(file, path, mode) file = fopen(path, mode)
	#define PATH_PRINT(...) printf(__VA_ARGS__)
	#define PATH_SPRINT(...) sprintf(__VA_ARGS__)

	#define min(a,b) ((a) < (b) ? a : b)
	#define max(a,b) ((a) > (b) ? a : b)

#endif

//#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
//#define MTYPEOF(T) typeof(T)
//#elif defined(__GNUC__) || defined(__clang__)
//#define MTYPEOF(T) __typeof__(T)
//#else 
//#error "Requires typeof() support, you must either use C23 or later, or the GCC or Clang compilers"
//#endif

typedef struct int2
{
	int32_t x;
	int32_t y;
} int2;

// Use this for exported functions instead of ivec4s. ivec4s doesn't enforce alignment, but
// all the other cglm vector types do and it makes sense that it should, so this might change in the future.  
typedef struct int4_unaligned
{
	int32_t v0[4];
} int4_unaligned;

// Use this for exported function parameters instead of vec4s to avoid 
// BAD_INSTRUCTION_PTR_INVALID_POINTER_READ due to the compiler expecting 
// the vec4s argument to be aligned on 16, and trying to use aligned vector
// instructions on an unaligned address
typedef struct float4_unaligned
{
	float v0[4];
} float4_unaligned;

// Use this for exported function parameters instead of mat4s to avoid 
// BAD_INSTRUCTION_PTR_INVALID_POINTER_READ due to the compiler expecting 
// the mat4s argument to be aligned on 32, and trying to use aligned vector
// instructions on an unaligned address 
typedef struct float4x4_unaligned
{
	float c0[4];
	float c1[4];
	float c2[4];
	float c3[4];
} float4x4_unaligned;

#if __STDC_VERSION__ || __cplusplus

#else
#error STDC not defined in platform?
#endif

#define Int32 int32_t
#define Int64 int64_t

#include "NativeShared.cs"

#undef Int32
#undef Int64


#define TXP_HANDLE_ERROR(TXPERR, TXPGOTO) if (TXPERR != TXP_RETURN_SUCCESS) {goto TXPGOTO;}
#define TXP_HANDLE_ERROR2(TXPERR, TXPERROUT, TXPGOTO) if (TXPERR != TXP_RETURN_SUCCESS) {TXPERROUT = TXPERR; goto TXPGOTO;}


typedef struct PathString
{
	int length;
	PATH_CHAR* s;
} PathString;

// The name of this shared library. I should probably do some environment variable bullshit to automatically determine the name
#define SHARED_LIB_NAME "SLZTextureProcessor.dll"
// Path to this shared library, used for locating config files and OpenCL kernels. On Windows this is a pointer to a UTF16 string
extern PathString* g_libraryPath;
extern void (*LogFn)(int level, char* message);

DLLEXPORT int32_t STDCALL InitializeLibPath(); // Finds path of this shared library and stores in 
DLLEXPORT void STDCALL PrintLibPath();

char* GetPlatShortPath(PATH_CHAR* longPath);

#ifdef PLATFORM_WINDOWS
PATH_CHAR* UTF8ToUTF16(char* utf8);
#endif

void FreePlatShortPath(char* shortPath);

#if defined(__clang__) || defined(__GNUC__)
#pragma omp declare simd
#endif
static inline int imax(int a, int b)
{
	return a > b ? a : b;
}
#if defined(__clang__) || defined(__GNUC__)
#pragma omp declare simd
#endif
static inline int imin(int a, int b)
{
	return a < b ? a : b;
}


void DebugLog(int level, char* message, ...);
const char* TXPErrorToStr(TXPErrorCode err);

#endif // SLZ_PLATFORM_HEADER