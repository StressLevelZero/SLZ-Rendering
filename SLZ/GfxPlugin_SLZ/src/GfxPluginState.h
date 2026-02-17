#ifndef GFX_PLUGIN_STATE
#define GFX_PLUGIN_STATE

#include "Platform.h"

#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityLog.h"
#include "PluginAPI.h"

#if defined(PLATFORM_ANDROID)
#include <android/log.h>
#endif

struct IUnityInterfaces;
struct IUnityGraphics;
enum UnityGfxRenderer;
class PluginAPI;

namespace GfxPlugin
{
public:
    static const char* s_PluginNameShort = "GfxPlugin";
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
#if defined(PLATFORM_WINDOWS)
            const char* logLevel;
            switch (type)
            {
            case (kUnityLogTypeError):      logLevel = "Error";     break;
            case (kUnityLogTypeWarning):    logLevel = "Warning";   break;
            case (kUnityLogTypeLog):        logLevel = "Log";       break;
            case (kUnityLogTypeException):  logLevel = "Exception"; break;
            }

            std::string message2 = std::format("{} {}: {}\nAt {}:{}", s_PluginNameShort, logLevel, message, fileName, fileLine);
            OutputDebugStringA(message2.c_str());
#elif defined(PLATFORM_ANDROID)
            android_LogPriority priority;
            switch (type)
            {
            case (kUnityLogTypeError):     priority = ANDROID_LOG_ERROR; break;
            case (kUnityLogTypeWarning):   priority = ANDROID_LOG_WARN;  break;
            case (kUnityLogTypeLog):       priority = ANDROID_LOG_INFO;  break;
            case (kUnityLogTypeException): priority = ANDROID_LOG_FATAL; break;
            }
            std::string message2 = std::format("{} : at {}:{}", message, fileName, fileLine);
            __android_log_print(priority, s_PluginNameShort, message2);
#endif
        }
    }
};

#endif

