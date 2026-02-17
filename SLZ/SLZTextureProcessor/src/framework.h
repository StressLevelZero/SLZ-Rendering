#ifndef FRAMEWORK
#define FRAMEWORK

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY) || defined(__CYGWIN__) || defined(__CYGWIN32__)
	#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
	// Windows Header Files
	#define _CRT_SECURE_NO_DEPRECATE // Allow using portable fprintf instead of windows's fprintf_s
	#include <windows.h>
#endif

#include <stdlib.h>
#endif

