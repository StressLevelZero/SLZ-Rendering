#define _GNU_SOURCE 1
#include <stdlib.h>
#include <locale.h>

#include "Platform.h"
#include "DllExport.h"


PathString* g_libraryPath = NULL;
void (*LogFn)(int level, char* message) = NULL;

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
#include <tchar.h>
#include <wchar.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#endif

/** @brief Find the path of this shared library and store it in the libraryPath global. 
*/
DLLEXPORT int32_t STDCALL InitializeLibPath()
{
#ifdef PLATFORM_WINDOWS
	HMODULE thisModule = GetModuleHandle(SHARED_LIB_NAME);
	if (thisModule == NULL)
	{
		return 1;
	}
	
	DWORD bufferSize = 257;
	int pathLength = 0;
	LPWSTR path = NULL;
	// blindly guess the path length until our buffer is large enough to contain the path since GetModuleFileName is dumb
	while (bufferSize < 32768)
	{
		DWORD allocLen = bufferSize * sizeof(WCHAR);
		path = malloc(allocLen);
		if (path == NULL) return 1;
		
		DWORD guessSize = GetModuleFileNameW(thisModule, path, allocLen);
		if (guessSize < bufferSize)
		{
			pathLength = guessSize + 1; // Needs NULL terminator
			
			LPWSTR path2 = malloc(pathLength * sizeof(WCHAR));
			memcpy_s(path2, pathLength * sizeof(WCHAR), path, pathLength * sizeof(WCHAR));
			free(path);
			path = path2;
			break;
		}

		bufferSize *= 2;
		free(path);
	}
	
#elif defined(PLATFORM_LINUX)
	Dl_info dlinfo = {};

	int errorCode = dladdr(InitializeLibPath, &dlinfo);

	if (errorCode != 0)
	{
		printf("Could not find library path!\n");
		if (LogFn != NULL) LogFn(0, "Could not find library path!\n");
		return 1;
	}

	int pathLength = strlen(dlinfo.dli_fname) + 1;
	PATH_CHAR* path = malloc(sizeof(PATH_CHAR) * pathLength);
	memcpy(path, dlinfo.dli_fname, pathLength * sizeof(PATH_CHAR));
	path[pathLength - 1] = (PATH_CHAR)0;
#endif

	int basePathLen = 0;
	for (int i = pathLength - 1; i >= 0; i--)
	{
		if (path[i] == PATH_SEPARATOR)
		{
			basePathLen = i + 1;
			path[i + 1] = (PATH_CHAR)0;
			break;
		}
	}

	g_libraryPath = malloc(sizeof(PathString));
	g_libraryPath->length = basePathLen;
	g_libraryPath->s = path;

	return 0;

}

DLLEXPORT void STDCALL PrintLibPath()
{ 
#if defined(PLATFORM_WINDOWS)
	wprintf(g_libraryPath->s);
	printf("\n");
#else
	printf("%s", g_libraryPath->s);
	printf("\n");
#endif
}

void DisposePlatformData()
{
	if (g_libraryPath != NULL)
	{
		if (g_libraryPath->s != NULL)
		{
			free(g_libraryPath->s);
		}
		free(g_libraryPath);
	}
}

char* GetPlatShortPath(PATH_CHAR* longPath)
{
#if defined(PLATFORM_WINDOWS)
	char* path;

	/*
	int wIdx = 0;
	int ASCII = 1;
	while (longPath[wIdx] != 0)
	{
		if (longPath[wIdx] > 127) // ASCII is 7 bits, first 127 values of UTF16 are the same as ASCII
		{
			ASCII = 0;
			break;
		}
		wIdx++;
	}
	if (ASCII)
	{
		path = malloc((wIdx + 1) * sizeof(char));
		path[wIdx] = '\0';
		for (int cIdx = 0; cIdx < )
	}
	*/
	
	/*
	int pathLen = 0;

	int ret = GetShortPathNameW(longPath, NULL, 0);
	if (ret <= 0)
	{
		path = (char*)malloc(sizeof(char));
		path[0] = '\0';
		return path;
	}
	PATH_CHAR* shortPath = (PATH_CHAR*)malloc(ret * sizeof(PATH_CHAR));
	int ret2 = GetShortPathNameW(longPath, shortPath, ret);
	path = malloc(ret * sizeof(char));
	for (int i = 0; i < ret; i++)
	{
		path[i] = (char)shortPath[i];
	}
	free(shortPath);
	*/
	size_t utf8Len = 0;
	_locale_t utf8Locale = _create_locale(LC_ALL, ".UTF8");
	errno_t errorCode = _wcstombs_s_l(&utf8Len, NULL, 0, longPath, 0, utf8Locale);
	if (errorCode != 0)
	{
		printf("Converting UTF16 path to UTF8 failed, error code: %d", errorCode);
		return "";
	}
	size_t pathSize = utf8Len * sizeof(char);
	path = (char*)malloc(pathSize);
	errorCode = _wcstombs_s_l(&utf8Len, path, pathSize, longPath, pathSize - sizeof(char), utf8Locale);
	if (errorCode != 0)
	{
		printf("Converting UTF16 path to UTF8 failed, error code: %d", errorCode);
		return "";
	}

	return path;
#else
	return longPath;
#endif
}

#ifdef PLATFORM_WINDOWS
PATH_CHAR* UTF8ToUTF16(char* utf8)
{
	size_t utf16Len = 0;
	_locale_t utf8Locale = _create_locale(LC_ALL, ".UTF8");
	errno_t errorCode = _mbstowcs_s_l(&utf16Len, NULL, 0, utf8, 0, utf8Locale);
	if (errorCode != 0) return L"";
	size_t pathSize = utf16Len * sizeof(wchar_t);
	wchar_t* utf16 = (wchar_t*)malloc(pathSize);
	errorCode = _mbstowcs_s_l(&utf16Len, utf16, pathSize / sizeof(WORD), utf8, utf16Len - 1, utf8Locale);
	if (errorCode != 0) return L"";
	return utf16;
}
#endif

void FreePlatShortPath(char* shortPath)
{
#if defined(PLATFORM_WINDOWS)
	free(shortPath);
#endif
}

void DebugLog(int level, char* message, ...)
{

	va_list ap;
	va_start(ap, message);
	int size = vsnprintf(NULL, 0, message, ap);
	if (size <= 0)
	{
		if (LogFn != NULL) LogFn(level, "vsprintf returned negative number!\n");
		return;
	}
	char* msg = (char*)malloc(size + 1);
	vsnprintf(msg, size + 1, message, ap);
	va_end(ap);
	if (LogFn != NULL)
	{
		LogFn(level, msg);
	}
	printf(msg);

	free(msg);
}

const char* TXPErrorToStr(TXPErrorCode err)
{
	switch (err)
	{
	case(TXP_RETURN_SUCCESS):             return "TXP_RETURN_SUCCESS"; break;
	case(TXP_RETURN_GENERAL_FAILURE):     return "TXP_RETURN_GENERAL_FAILURE"; break;
	case(TXP_RETURN_ALLOC_FAILED):        return "TXP_RETURN_ALLOC_FAILED"; break;
	case(TXP_RETURN_INVALID_ARGS):        return "TXP_RETURN_INVALID_ARGS"; break;
	case(TXP_RETURN_INVALID_TEX_FORMAT):  return "TXP_RETURN_INVALID_TEX_FORMAT"; break;
	case(TXP_RETURN_ZERO_SIZE_TEX):       return "TXP_RETURN_ZERO_SIZE_TEX"; break;
	case(TXP_RETURN_IMG_LIBRARY_FAILED):  return "TXP_RETURN_IMG_LIBRARY_FAILED"; break;
	case(TXP_RETURN_NO_IMG_LIBRARY):      return "TXP_RETURN_NO_IMG_LIBRARY"; break;
	case(TXP_RETURN_INVALID_IMG_LIBRARY): return "TXP_RETURN_INVALID_IMG_LIBRARY"; break;
	case(TXP_RETURN_INVALID_PATH):        return "TXP_RETURN_INVALID_PATH"; break;
	case(TXP_RETURN_FILE_OPEN_FAILED):    return "TXP_RETURN_FILE_OPEN_FAILED"; break;
	default: break;
	}
	return "(Unhandled Error Code)";
}

