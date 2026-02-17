#ifndef SLZ_PLATFORM_HEADER
#define SLZ_PLATFORM_HEADER

#include <stdint.h>
#include <memory>
#include <filesystem>

// Remove disastrous C++20 utf8 types. 
// Visual studio doesn't parse command-line arguments before doing syntax checking in IDE, so I have to manually undef this
#ifdef __cpp_lib_char8_t
#undef __cpp_lib_char8_t
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__MINGW64)
    #define VK_USE_PLATFORM_WIN32_KHR
    #define PLATFORM_WINDOWS
    #define PLATFORM_PC
    
    // Windows Header Files
    #define _CRT_SECURE_NO_DEPRECATE // Allow using portable fprintf instead of windows's fprintf_s 
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <fileapi.h>
    #include <string>
    #define PATH_CHAR wchar_t
    #define PATH_STRING std::wstring 
    #define PATH_LITERAL(val) L##val
    #define FILE_OPEN(file, path, mode) file = _wfsopen(path, mode, 0x40)
    #define PATH_PRINT(...) wprintf(__VA_ARGS__)
    #define PATH_SPRINT(...) swprintf(__VA_ARGS__)

    #define SHARED_LIBRARY_TYPE HMODULE 
    #define OPEN_SHARED_LIBRARY(library) LoadLibrary(library)
    #define LOAD_FROM_LIBRARY(library, funcName) GetProcAddress(library, funcName)
    #define CLOSE_SHARED_LIBRARY(library) FreeLibrary(library)

    #define STDCALL  __stdcall
    #define DLLEXPORT __declspec(dllexport)
    #define WINDOWS_OPEN_FILE_SHARED , _SH_DENYNO
#elif defined(__linux__)
    #if defined(__ANDROID__)
        #define VK_USE_PLATFORM_ANDROID_KHR
        #define PLATFORM_ANDROID
        #define PLATFORM_MOBILE
    #else
        #define PLATFORM_LINUX
        #define PLATFORM_PC
    #endif

    // linux header files
    #include <dlfcn.h>
    #include <string.h>
    #define PATH_CHAR char
    #define PATH_STRING std::string 
    #define PATH_LITERAL(val) val
    #define FILE_OPEN(file, path, mode) file = fopen(path, mode)
    #define PATH_PRINT(...) printf(__VA_ARGS__)
    #define PATH_SPRINT(...) sprintf(__VA_ARGS__)

    #define SHARED_LIBRARY_TYPE void* 
    #define OPEN_SHARED_LIBRARY(library) dlopen(library, RTLD_NOW)
    #define LOAD_FROM_LIBRARY(library, funcName) dlsym(library, funcName)
    #define CLOSE_SHARED_LIBRARY(library) dlclose(library)

    #define STDCALL
    #define DLLEXPORT __attribute__((visibility("default")))
    #define WINDOWS_OPEN_FILE_SHARED 
#endif

PATH_STRING GetLibAddress(int& errorCode);

#endif // SLZ_PLATFORM_HEADER
