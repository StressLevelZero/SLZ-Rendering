#ifndef SLZ_DEBUG_LOG
#define SLZ_DEBUG_LOG

#include <string>
#include "IUnityInterface.h"

using namespace std;
using std::string;

//Pointer to a function in a unity C# class that handles printing to the debug log
typedef void(__stdcall *UnityDebugLogFuncPtr)(int level, const char*);

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API BindLogger(UnityDebugLogFuncPtr logPtr);

class DebugLog
{
public:
	static void SetLogPointer(UnityDebugLogFuncPtr logPtr);
	static void ClearLogPointer();
	static bool hasLogPointer();
	static void Log(const char* message);
	static void LogWarning(const char* message);
	static void LogError(const char* message);
	static void LogWinDbgString(const char* message);
protected:
	static UnityDebugLogFuncPtr debugLog;
};

#endif
