// Platform Headers
#include "Platform.h"

// Library Headers
#include <vector>
#include <string>
#include <fstream>
#include <format>
#include <filesystem>

#if defined(PLATFORM_WINDOWS)
#include <processenv.h>
#include <shellapi.h>
#endif

#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityLog.h"

// Local Headers
#include "PluginState.h"
#include "PluginApiVk.h"
#include "PluginApiNone.h"
#include "ApplicationPaths.h"
#include "SLZGraphicsConfig.h"


static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);
static void UNITY_INTERFACE_API OnRenderEventWithData(int eventID, void* data);
static void UNITY_INTERFACE_API OnRenderEvent(int eventID);
static PluginAPI* GetPlatformAPI(UnityGfxRenderer renderer);
static int GetCmdlineArgs(SLZGraphicsConfig* config);


#ifdef PLATFORM_WINDOWS
extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        int errorCode = 0;
        ApplicationPaths::libraryPath = ApplicationPaths::WinGetLibAddress(hModule, errorCode);
        if (errorCode != 0)
        {
            PluginState::Log(kUnityLogTypeException, "Failed to get library path!", __FILE__, __LINE__);
            //std::abort();
            return FALSE;
        }
        else
        {
            PluginState::Log(kUnityLogTypeLog, std::format("Got library path: {}", (char*)(ApplicationPaths::libraryPath.u8string().c_str())).c_str(), __FILE__, __LINE__);
        }
        break;
    }
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}
#endif

static int GetCmdlineArgs(SLZGraphicsConfig* config)
{
#ifdef PLATFORM_WINDOWS
    LPWSTR lpCmdArgs = GetCommandLineW();
    int argCnt = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdArgs, &argCnt);
    for (int i = 0; i < argCnt; i++)
    {
        if (wcscmp(argv[i], L"-nogfxnative") == 0)
        {
            return 1;
        }
        if (wcscmp(argv[i], L"-batchmode") == 0)
        {
            PluginState::Log(kUnityLogTypeWarning,
                "SLZ Graphics Plugin: Detected running in batch mode! Disabling vkCreateSampler hook as that can cause a crash on fresh project imports!",
                __FILE__,
                __LINE__
            );
            config->disableSamplerHook = true;
        }
    }
    LocalFree(argv);
    
#endif
    return 0;
}

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces * unityInterfaces)
{
    PluginState::s_UnityLog = unityInterfaces->Get<IUnityLog>();

    int errorCode = ApplicationPaths::PopulatePathsAndEditor();
    PluginState::s_IsEditor = ApplicationPaths::isEditor;

    SLZGraphicsConfig::s_Instance = new SLZGraphicsConfig();

    // Always enable 2x2 shading rate in editor since we can't swap after initialization
    if (PluginState::s_IsEditor)
    {
        SLZGraphicsConfig::allow2x2ShadingHack = true;
    }

    if (!ApplicationPaths::peristentDataPath.empty())
    {
        //PluginState::Log(kUnityLogTypeLog, FileSystemWin::s_LibPath.u8string().c_str(), __FILE__, __LINE__);
        std::filesystem::path configPath2 = ApplicationPaths::peristentDataPath / "graphics.ini";
        std::ifstream graphicsIni(configPath2, std::ios_base::in WINDOWS_OPEN_FILE_SHARED);
        if (!graphicsIni.fail())
        {
            SLZGraphicsConfig::s_Instance->ReadSettingsIni(&graphicsIni);
        }
        else
        {
            //PluginState::Log(kUnityLogTypeWarning, "Failed to open Graphics.ini", __FILE__, __LINE__);
            PluginState::Log(kUnityLogTypeWarning, std::format("SLZ Graphics Plugin: Failed to find graphics.ini at {}", (const char*)(configPath2.u8string().c_str())).c_str(), __FILE__, __LINE__);
        }
        PluginState::Log(kUnityLogTypeLog, SLZGraphicsConfig::s_Instance->PrintSettings().c_str(), __FILE__, __LINE__);
    }

    // Further configure/override settings based on command line arguments
    int disablePlugin = GetCmdlineArgs(SLZGraphicsConfig::s_Instance);
    if (disablePlugin)
    {
        PluginState::api = nullptr;
        return;
    }

    PluginState::s_UnityInterfaces = unityInterfaces;
    PluginState::api = nullptr;
    PluginState::s_Graphics = PluginState::s_UnityInterfaces->Get<IUnityGraphics>();
    PluginState::s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);




#ifndef DUMMY_PLUGIN
    if (PluginState::s_Graphics->GetRenderer() == kUnityGfxRendererNull || PluginState::s_Graphics->GetRenderer() == kUnityGfxRendererVulkan)
    {
        RenderAPI_Vulkan_OnPluginLoad(unityInterfaces);
        PluginState::Log(kUnityLogTypeLog, "Called RenderAPI_Vulkan_OnPluginLoad", __FILE__, __LINE__);
    }
    else
    {
        PluginState::Log(kUnityLogTypeLog, "Renderer NOT vulkan!", __FILE__, __LINE__);
    }
#endif

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    if (PluginState::s_Graphics)
    {
        PluginState::s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
        
        //s_Graphics = nullptr;
    }
    delete(SLZGraphicsConfig::s_Instance);
}


static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {
        case kUnityGfxDeviceEventInitialize:
            PluginState::s_Renderer = PluginState::s_Graphics->GetRenderer();
            PluginState::api = GetPlatformAPI(PluginState::s_Renderer);
            PluginState::api->GfxEventInit();
            break;
        case kUnityGfxDeviceEventBeforeReset:
            break;
        case kUnityGfxDeviceEventAfterReset:
            break;
        case kUnityGfxDeviceEventShutdown:
            if (PluginState::api)
            {
                PluginState::api->GfxEventShutdown();
            }
            //PluginState::s_UnityInterfaces = nullptr;
            //PluginState::s_Graphics = nullptr;
            //PluginState::s_SupportsNvVRS = false;
            //PluginState::s_Renderer = kUnityGfxRendererNull;
            //delete(PluginState::api);
            break;
    }
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
    return OnRenderEvent;
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventWithDataFunc()
{
    return OnRenderEventWithData;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API PrintStatusMessage()
{
    if (PluginState::api != nullptr)
    {
        std::string message = PluginState::api->GetStatusMessage();
        PluginState::Log(kUnityLogTypeLog, message.c_str(), __FILE__, __LINE__);
    }
}

static PluginAPI* GetPlatformAPI(UnityGfxRenderer renderer)
{
#if DUMMY_PLUGIN
    return CreateApiUnknown();
#endif
    switch (renderer)
    {
    case (kUnityGfxRendererVulkan):
        return CreateApiVk();
    default:
        return CreateApiUnknown();
    }
}

static void UNITY_INTERFACE_API OnRenderEventWithData(int eventID, void* data)
{
    PluginState::api->OnRenderEventWithData(eventID, data);
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
    PluginState::api->OnRenderEvent(eventID);
}

