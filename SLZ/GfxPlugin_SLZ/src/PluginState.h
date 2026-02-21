#ifndef SLZ_PLUGIN_STATE
#define SLZ_PLUGIN_STATE

#include "IUnityInterface.h"
#include "Platform.h"

#include "IUnityGraphics.h"
#include "IUnityLog.h"
#include "PluginAPI.h"
#include <filesystem>
#include <iostream>
#include <format>

#if defined(PLATFORM_ANDROID)
#include <android/log.h>
#endif


class PluginAPI;

class PluginState
{
public:
    static inline const char* s_PluginNameShort = "GfxPlugin";
	static inline IUnityInterfaces* s_UnityInterfaces = nullptr;
	static inline IUnityGraphics* s_Graphics = nullptr;
	static inline IUnityLog* s_UnityLog = nullptr;
	static inline UnityGfxRenderer s_Renderer = kUnityGfxRendererNull;
	static inline PluginAPI* api = nullptr;
	static inline bool s_IsEditor = false;
	static void Log(UnityLogType type, const char* message, const char* fileName, const int fileLine)
	{
		if (s_UnityLog != nullptr)
		{
			s_UnityLog->Log(type, message, fileName, fileLine);
		}
		else
		{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
            const char* logLevel;
            switch (type)
            {
            case (kUnityLogTypeError):      logLevel = "Error";     break;
            case (kUnityLogTypeWarning):    logLevel = "Warning";   break;
            case (kUnityLogTypeLog):        logLevel = "Log";       break;
            case (kUnityLogTypeException):  logLevel = "Exception"; break;
            }

            std::string message2 = std::format("{} {}: {}\nAt {}:{}", s_PluginNameShort, logLevel, message, fileName, fileLine);
#if defined(PLATFORM_WINDOWS)
            OutputDebugStringA(message2.c_str());
#endif
            std::cout << message2;
#elif defined(PLATFORM_ANDROID)
            android_LogPriority priority;
            switch (type)
            {
            case (kUnityLogTypeError):     priority = ANDROID_LOG_ERROR; break;
            case (kUnityLogTypeWarning):   priority = ANDROID_LOG_WARN;  break;
            case (kUnityLogTypeLog):       priority = ANDROID_LOG_INFO;  break;
            case (kUnityLogTypeException): priority = ANDROID_LOG_FATAL; break;
            }
            //std::string message2 = std::format("{} : at {}:{}", message, fileName, fileLine);
            __android_log_print(priority, s_PluginNameShort, "%s at %s : line %d", message, fileName, fileLine);
#endif
		}
	}
};

#endif

