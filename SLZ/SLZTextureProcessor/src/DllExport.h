#ifndef DLL_EXPORT
#define DLL_EXPORT

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY) || defined(__CYGWIN__) || defined(__CYGWIN32__)
	#define STDCALL  __stdcall
	#define DLLEXPORT __declspec(dllexport)
#else
	#define STDCALL
	#define DLLEXPORT __attribute__((visibility("default")))
#endif

#endif
