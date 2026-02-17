#ifndef SLZ_VRS_VK
#define SLZ_VRS_VK

struct IUnityInterfaces;
class PluginAPI;

PluginAPI* CreateApiVk();
extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces * interfaces);

#endif