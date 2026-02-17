#include "DebugLog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
//#define OutputDebugStringA(message)

UnityDebugLogFuncPtr DebugLog::debugLog = nullptr;



extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API BindLogger(UnityDebugLogFuncPtr logPtr)
{
	DebugLog::SetLogPointer(logPtr);
}

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API DisposeLogger()
{
	DebugLog::ClearLogPointer();
}

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API PrintDebugMessage()
{
	DebugLog::Log("SLZ VRS Test message");
}



void DebugLog::SetLogPointer(UnityDebugLogFuncPtr logPtr)
{
	debugLog = logPtr;
}

void DebugLog::ClearLogPointer()
{
	debugLog = nullptr;
}

bool DebugLog::hasLogPointer()
{
	return debugLog != nullptr;
}

void DebugLog::LogWinDbgString(const char* message)
{
	OutputDebugStringA(message);
}

void DebugLog::Log(const char* message)
{
	if (debugLog != nullptr)
	{
		debugLog((int)LogLevel::DLMESSAGE, message);
	}
	else
	{
		OutputDebugStringA(message);
	}
}

void DebugLog::LogWarning(const char* message)
{
	if (debugLog != nullptr)
	{
		debugLog((int)LogLevel::DLWARNING, message);
	}
	else
	{
		OutputDebugStringA(message);
	}
}

void DebugLog::LogError(const char* message)
{
	if (debugLog != nullptr)
	{
		debugLog((int)LogLevel::DLERROR, message);
	}
	else
	{
		OutputDebugStringA(message);
	}
}

