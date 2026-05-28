#include "PluginApiVk.h"
#include "Platform.h"

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <processenv.h>
#include <shellapi.h>
#endif

#if !defined(PLATFORM_WINDOWS)
#include "alloca.h"
#define _malloca(size) alloca(size)
#define _freea(ptr)
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <unordered_set>


#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"

#include "PluginState.h"
#include "SLZGraphicsConfig.h"

using namespace std;

//#define NVAPI_VRS
//#define DUMMY_PLUGIN
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Macro soup from the unity's example native vulkan plugin, declares static 
// pointers to used vulkan functions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// This macro calls the given macro for each of the listed vulkan function names
#define UNITY_USED_VULKAN_API_FUNCTIONS(macro) \
    macro(vkCreateInstance); \
    macro(vkCreateDevice); \
    macro(vkGetDeviceProcAddr); \
    macro(vkGetPhysicalDeviceFeatures2); \
    macro(vkGetPhysicalDeviceProperties2); \
    macro(vkEnumerateDeviceExtensionProperties); \
    macro(vkGetPhysicalDeviceFormatProperties); \
    macro(vkGetPhysicalDeviceMemoryProperties); \
    macro(vkGetPhysicalDeviceQueueFamilyProperties); \
    macro(vkDestroyImageView); \
    

#define UNITY_USED_VULKAN_DEVICE_FUNCTIONS(macro) \
    macro(vkAllocateCommandBuffers); \
    macro(vkAllocateMemory); \
    macro(vkBeginCommandBuffer); \
    macro(vkBindBufferMemory); \
    macro(vkBindImageMemory); \
    macro(vkCmdBindShadingRateImageNV); \
    macro(vkCmdCopyBufferToImage); \
    macro(vkCmdPipelineBarrier); \
    macro(vkCmdSetFragmentShadingRateKHR); \
    macro(vkCmdSetShadingRateImageEnableNV); \
    macro(vkCmdSetViewportShadingRatePaletteNV); \
    macro(vkCreateBuffer); \
    macro(vkCreateCommandPool); \
    macro(vkCreateFramebuffer);\
    macro(vkCreateGraphicsPipelines);\
    macro(vkCreateImage); \
    macro(vkCreateImageView); \
    macro(vkCreateRenderPass); \
    macro(vkCreateRenderPass2); \
    macro(vkCreateRenderPass2KHR);\
    macro(vkCreateSampler); \
    macro(vkDestroyBuffer); \
    macro(vkDestroyCommandPool); \
    macro(vkGetImageMemoryRequirements); \
    macro(vkGetBufferMemoryRequirements); \
    macro(vkMapMemory); \
    macro(vkUnmapMemory); \
    macro(vkEndCommandBuffer); \
    macro(vkFreeCommandBuffers); \
    macro(vkQueueSubmit); \
    macro(vkQueueWaitIdle); \
    



    
    

//This macro creates a static function pointer for a given function name, prefixing it with "p_" to avoid conflicts with the actual function if VK_NO_PROTOTYPES isn't defined
#define VULKAN_DEFINE_API_FUNCPTR(func) static PFN_##func p_##func = (PFN_##func)nullptr;

    // Create a pointer "p_vkGetInstanceProcAddr" for the vkGetInstanceProcAddr function 
    VULKAN_DEFINE_API_FUNCPTR(vkGetInstanceProcAddr);

    // Create pointers for all of the functions listed under UNITY_USED_VULKAN_API_FUNCTIONS macro defined above
    UNITY_USED_VULKAN_API_FUNCTIONS(VULKAN_DEFINE_API_FUNCPTR)
    UNITY_USED_VULKAN_DEVICE_FUNCTIONS(VULKAN_DEFINE_API_FUNCPTR)

#undef VULKAN_DEFINE_API_FUNCPTR


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Forward function declarations
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName);
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetDeviceProcAddr(VkDevice device, const char* funcName);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);
static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);

static void LoadVulkanAPI(PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkInstance instance);
static void LoadVulkanAPIDevice(PFN_vkGetDeviceProcAddr getDeviceProcAddr, VkDevice device);
static void LoadVulkanAPIDeviceFromInstance(VkInstance instance);

static VkResult CreateRenderPassWithShadingRate(VkDevice device,
    const VkRenderPassCreateInfo2* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass);

typedef struct UnityFoveatedInfo
{
    uint64_t size = 128;
    uint32_t unknown1;
    uint16_t unknown2;
    uint16_t depth;
    uint32_t width;
    uint32_t height;
    float unknown3 = 0.5;
    float unknown4;
    float unknown5;
    float unknown6;
    float unknown7;
    void* colorBufferPointer;
} UnityFoveatedInfo;

typedef enum PcieVendorIDs : uint32_t
{
    PCIE_VENDOR_ID_AMD      = 0x1002u,
    PCIE_VENDOR_ID_NVIDIA   = 0x10DEu,
    PCIE_VENDOR_ID_ARM      = 0x13B5u,
    PCIE_VENDOR_ID_QCOM     = 0x5143u,
    PCIE_VENDOR_ID_INTEL    = 0x8086u
} VkVendorIDs;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Unity Interface
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API InterceptVulkanInitialization(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void*)
{
    PluginState::Log(kUnityLogTypeLog, "InterceptVulkanInitialization was called!", __FILE__, __LINE__);
    p_vkGetInstanceProcAddr = getInstanceProcAddr;
    return Hook_vkGetInstanceProcAddr;
}

extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces * interfaces)
{
    if (IUnityGraphicsVulkanV2* vulkanInterface = interfaces->Get<IUnityGraphicsVulkanV2>())
    {
        bool success = vulkanInterface->AddInterceptInitialization(InterceptVulkanInitialization, NULL, 0);
        if (!success)
        {
            PluginState::Log(kUnityLogTypeError, "AddInterceptInitialization failed, called too late????", __FILE__, __LINE__);
        }
    }
    else
    {
        PluginState::Log(kUnityLogTypeError, "RenderAPI_Vulkan_OnPluginLoad did not get an IUnityGraphicsVulkanV2 interface!", __FILE__, __LINE__);
    }
}

static void LoadVulkanAPI(PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkInstance instance)
{
    if (!p_vkGetInstanceProcAddr && getInstanceProcAddr)
        p_vkGetInstanceProcAddr = getInstanceProcAddr;

    if (!p_vkCreateInstance)
        p_vkCreateInstance = (PFN_vkCreateInstance)p_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");

    // sets the static pointers of each of the vulkan functions listed under the UNITY_USED_VULKAN_API_FUNCTIONS macro
    // from the pointer returned by p_vkGetInstanceProcAddr for the given instance and function name
    // Original implementation did a null check on the pointer, but unity editor can reload the graphics system while running (ie loading renderdoc).
    // The function pointers need to be refreshed at that point otherwise they won't go through any addtional layers that got added and will potentially cause a crash
#define LOAD_VULKAN_FUNC(fn) p_##fn = (PFN_##fn)p_vkGetInstanceProcAddr(instance, #fn)
    UNITY_USED_VULKAN_API_FUNCTIONS(LOAD_VULKAN_FUNC);
#undef LOAD_VULKAN_FUNC
}

static void LoadVulkanAPIDevice(PFN_vkGetDeviceProcAddr getDeviceProcAddr, VkDevice device)
{
    if (!p_vkGetDeviceProcAddr && getDeviceProcAddr)
        p_vkGetDeviceProcAddr = getDeviceProcAddr;

    // sets the static pointers of each of the vulkan functions listed under the UNITY_USED_VULKAN_API_FUNCTIONS macro
    // from the pointer returned by p_vkGetDeviceProcAddr for the given device and function name
#define LOAD_VULKAN_DEVICE_FUNC(fn) p_##fn = (PFN_##fn)p_vkGetDeviceProcAddr(device, #fn)
    UNITY_USED_VULKAN_DEVICE_FUNCTIONS(LOAD_VULKAN_DEVICE_FUNC);
#undef LOAD_VULKAN_DEVICE_FUNC
}

// Unity appears to be incorrectly loading ALL functions from vkGetInstanceProcAddr, trying to get our functions from the device results in hooking conflicts
// Thus we need to also load device-level functions via the instance. EDIT - Added redirection of vkGetInstanceProcAddr to vkGetDeviceProcAddr inside of
// Hook_vkGetInstanceProcAddr after the device hase been created. Thus we can use LoadVulkanAPIDevice instead of this.
static void LoadVulkanAPIDeviceFromInstance(VkInstance instance)
{
    // sets the static pointers of each of the vulkan functions listed under the UNITY_USED_VULKAN_API_FUNCTIONS macro
    // from the pointer returned by p_vkGetInstanceProcAddr for the given instance and function name
#define LOAD_VULKAN_DEVICE_FUNC(fn) p_##fn = (PFN_##fn)p_vkGetInstanceProcAddr(instance, #fn)
    UNITY_USED_VULKAN_DEVICE_FUNCTIONS(LOAD_VULKAN_DEVICE_FUNC);
#undef LOAD_VULKAN_DEVICE_FUNC
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Vulkan Plugin Class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


class PluginVk : public PluginAPI
{
public:
    // Attributes
    static inline PluginVk* instance = nullptr;
    static inline VkDevice vkDevice;
    static inline uint32_t s_vendorID;
    static inline bool s_SupportsVRSKHR = false;
    static inline bool s_SupportsBlendOpAdv = false;
    static inline bool s_InterceptImageCreation = false;
    static inline bool s_Shutdown = false;
    static inline bool s_SupportsPerPipelineShadingRate = false;
    static inline bool s_SupportsLayeredShadingRate = false;
    static inline VkExtent2D s_tileSize = { 1, 1 };

   
    
    static inline UnityVulkanInstance s_UnityInstanceVk = {};
    static inline VkPhysicalDevice s_PhysDevice = {};
    
#if defined(NVAPI_VRS)
    static inline bool s_UseNvApi = true;
#endif
    static inline uint32_t s_NvShadingRatePaletteCount = 0;
    /*
    static inline VkShadingRatePaletteEntryNV s_ShadingRateLUTStatic[16] = { 
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV, 
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV, 
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV
        };
        */
    
    static inline VkShadingRatePaletteEntryNV s_ShadingRateLUTStatic[16] = { 
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
        VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV
    };

    static inline VkShadingRatePaletteNV s_ShadingRatePalette[1] = {{16u, s_ShadingRateLUTStatic}};

    static inline VkPipelineViewportShadingRateImageStateCreateInfoNV s_NvPipelineExt =
    {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV,
        nullptr,
        true,
        1,
        s_ShadingRatePalette
    };

    static inline VkPipelineColorBlendAdvancedStateCreateInfoEXT s_AdvancedBlendEXT = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT,
        .pNext = NULL,
        .srcPremultiplied = VK_TRUE,
        .dstPremultiplied = VK_TRUE,
        .blendOverlap = VK_BLEND_OVERLAP_UNCORRELATED_EXT
    };

    static inline VkPipelineFragmentShadingRateStateCreateInfoKHR* s_PipelineRate = nullptr;
    static inline VkFragmentShadingRateAttachmentInfoKHR* s_SrAttachmentInfo = nullptr;
    static inline VkPhysicalDeviceFragmentShadingRatePropertiesKHR* s_VrsProps = nullptr;
    static inline IUnityGraphicsVulkan* s_UnityVulkan = nullptr;
    static inline IUnityGraphicsVulkanV2* s_UnityVulkan2 = nullptr;
    static const inline VkAllocationCallbacks* s_UnityAllocatorCallbacks = nullptr;

    VkCommandPool gfxCommandPool;

    UnityFoveatedInfo unityFoveatedInfo = {};

    VkAttachmentReference2* srAttachment = nullptr;
    
    VkExtent2D                         s_fragmentRateMax = { 1, 1 };
    VkFragmentShadingRateCombinerOpKHR s_CombinerOpsOn[2] = { VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR };
    VkFragmentShadingRateCombinerOpKHR s_CombinerOpsOff[2] = { VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR };

    VkExtent2D                         s_testFragmentSize = { 1, 1 };
    VkPipelineFragmentShadingRateStateCreateInfoKHR defaultShadingRate = { 
        VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR, 
        NULL,
        s_testFragmentSize,
        {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR} };
    VkPipelineFragmentShadingRateStateCreateInfoKHR vrsShadingRate = {
        VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR,
        NULL,
        s_testFragmentSize,
        {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR} };

    static inline VkPipelineFragmentShadingRateStateCreateInfoKHR s_pipelineRate2x2 = {
       VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR,
       NULL,
       {2, 2},
       {VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR}
    };


    struct ImageViewManager
    {
        VkImageView imageView;
        void* nativeTexPtr;
        void* nativeBufferPtr;

        unsigned long long lastUsedFrameNumber;
        bool markForDelete;
    };

    vector<ImageViewManager*>ImageViewDestroyQueue;
    ImageViewManager* currentSRImageView = NULL;
    bool enableShadingRateAttachment = false;

    uint32_t vrsWidth = 0;
    int32_t renderpassExecuted = 0;
    bool debugFrameBuffer = false;

    // Methods
    virtual void GfxEventInit();
    virtual void GfxEventShutdown();
    virtual string GetStatusMessage();

    virtual uint32_t GetShadingRateBlockSize();
    uint32_t GetMaxFragmentSize();
    uint32_t SupportsLayeredShadingRate() { return s_SupportsLayeredShadingRate; }
private:

};

PluginAPI* CreateApiVk()
{
    return new PluginVk();
}

void PluginVk::GfxEventInit()
{
    if (PluginState::s_UnityInterfaces && (PluginState::s_Graphics->GetRenderer() == kUnityGfxRendererVulkan)) {
        PluginVk::s_UnityVulkan2 = PluginState::s_UnityInterfaces->Get<IUnityGraphicsVulkanV2>();
        if (PluginVk::s_UnityVulkan2)
        {
            PluginVk::s_UnityInstanceVk = PluginVk::s_UnityVulkan2->Instance();
        }
        else
        {
            PluginState::Log(kUnityLogTypeException, "FATAL: Failed to Get IUnityGraphicsVulkanV2 interface from Unity!", __FILE__, __LINE__);
            return;
        }

       

        // Make sure Vulkan API functions are loaded
        // LoadVulkanAPI(PluginVk::s_UnityInstanceVk.getInstanceProcAddr, PluginVk::s_UnityInstanceVk.instance);
        // LoadVulkanAPIDeviceFromInstance(PluginVk::s_UnityInstanceVk.instance);

        PluginVk::instance = this;

        UnityVulkanPluginEventConfig config_1;
        config_1.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_DontCare;
        config_1.renderPassPrecondition = kUnityVulkanRenderPass_EnsureOutside;
        config_1.flags = kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;



        PluginState::Log(kUnityLogTypeLog, std::format("Supports advanced blend ops {}", s_SupportsBlendOpAdv).c_str(), __FILE__, __LINE__);
    }
}

void PluginVk::GfxEventShutdown()
{

    if (PluginVk::s_UnityInstanceVk.device != VK_NULL_HANDLE)
    {
        //GarbageCollect(true);
        //if (m_TrianglePipeline != VK_NULL_HANDLE)
        //{
        //	vkDestroyPipeline(m_Instance.device, m_TrianglePipeline, NULL);
        //	m_TrianglePipeline = VK_NULL_HANDLE;
        //}
        //if (m_TrianglePipelineLayout != VK_NULL_HANDLE)
        //{
        //	vkDestroyPipelineLayout(m_Instance.device, m_TrianglePipelineLayout, NULL);
        //	m_TrianglePipelineLayout = VK_NULL_HANDLE;
        //}
    }
    //PluginVk::s_UnityVulkan = NULL;
    //PluginVk::s_UnityVulkan2 = NULL;
    s_Shutdown = true;
    PluginVk::s_UnityInstanceVk = UnityVulkanInstance();
    if (PluginVk::s_PipelineRate)
    {
        free(PluginVk::s_PipelineRate);
    }
    if (PluginVk::s_VrsProps)
    {
        free(PluginVk::s_VrsProps);
    }
    if (PluginVk::instance->gfxCommandPool != VK_NULL_HANDLE)
    {
        p_vkDestroyCommandPool(PluginVk::s_UnityInstanceVk.device, PluginVk::instance->gfxCommandPool, NULL);
    }

}



string PluginVk::GetStatusMessage()
{

    string message = "";
    if ((PluginVk::s_VrsProps != nullptr))
    {
        message += "Device shading rate properties\n";
        message += "  minFragmentShadingRateAttachmentTexelSize ------------ " + to_string(PluginVk::s_VrsProps->minFragmentShadingRateAttachmentTexelSize.width) + ", " + to_string(PluginVk::s_VrsProps->minFragmentShadingRateAttachmentTexelSize.height) + "\n";
        message += "  maxFragmentShadingRateAttachmentTexelSize              " + to_string(PluginVk::s_VrsProps->maxFragmentShadingRateAttachmentTexelSize.width) + ", " + to_string(PluginVk::s_VrsProps->maxFragmentShadingRateAttachmentTexelSize.height) + "\n";
        message += "  maxFragmentShadingRateAttachmentTexelSizeAspectRatio - " + to_string(PluginVk::s_VrsProps->maxFragmentShadingRateAttachmentTexelSizeAspectRatio) + "\n";
        message += "  primitiveFragmentShadingRateWithMultipleViewports      " + to_string(PluginVk::s_VrsProps->primitiveFragmentShadingRateWithMultipleViewports) + "\n";
        message += "  layeredShadingRateAttachments ------------------------ " + to_string(PluginVk::s_VrsProps->layeredShadingRateAttachments) + "\n";
        message += "  fragmentShadingRateNonTrivialCombinerOps               " + to_string(PluginVk::s_VrsProps->fragmentShadingRateNonTrivialCombinerOps) + "\n";
        message += "  maxFragmentSize -------------------------------------- " + to_string(PluginVk::s_VrsProps->maxFragmentSize.width) + ", " + to_string(PluginVk::s_VrsProps->maxFragmentSize.height) + "\n";
        message += "  maxFragmentSizeAspectRatio                             " + to_string(PluginVk::s_VrsProps->maxFragmentSizeAspectRatio) + "\n";
        message += "  maxFragmentShadingRateCoverageSamples ---------------- " + to_string(PluginVk::s_VrsProps->maxFragmentShadingRateCoverageSamples) + "\n";
        message += "  maxFragmentShadingRateRasterizationSamples             " + to_string(PluginVk::s_VrsProps->maxFragmentShadingRateRasterizationSamples) + "\n";
        message += "  fragmentShadingRateWithShaderDepthStencilWrites ------ " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithShaderDepthStencilWrites) + "\n";
        message += "  fragmentShadingRateWithSampleMask                      " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithSampleMask) + "\n";
        message += "  fragmentShadingRateWithShaderSampleMask -------------- " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithShaderSampleMask) + "\n";
        message += "  fragmentShadingRateWithConservativeRasterization       " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithConservativeRasterization) + "\n";
        message += "  fragmentShadingRateWithFragmentShaderInterlock ------- " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithFragmentShaderInterlock) + "\n";
        message += "  fragmentShadingRateWithCustomSampleLocations           " + to_string(PluginVk::s_VrsProps->fragmentShadingRateWithCustomSampleLocations) + "\n";
        message += "  fragmentShadingRateStrictMultiplyCombiner ------------ " + to_string(PluginVk::s_VrsProps->fragmentShadingRateStrictMultiplyCombiner) + "\n";
    }
    return message;
}

uint32_t PluginVk::GetShadingRateBlockSize()
{
    return (s_tileSize.height << 16) | (s_tileSize.width);
}

uint32_t PluginVk::GetMaxFragmentSize()
{
    return (s_VrsProps->maxFragmentSize.height << 16) | (s_VrsProps->maxFragmentSize.width);
}



const std::unordered_set<std::string_view> instanceLoadedFuncs 
{
    "vkEnumerateInstanceVersion",
    "vkEnumerateInstanceExtensionProperties",
    "vkEnumeratePhysicalDevices",
    "vkEnumeratePhysicalDeviceGroups",
    "vkEnumeratePhysicalDeviceGroupsKHR",
    "vkCreateInstance",
    "vkDestroyInstance",
    "vkCreateAndroidSurfaceKHR",
    "vkCreateWaylandSurfaceKHR",
    "vkCreateWin32SurfaceKHR",
    "vkCreateXcbSurfaceKHR",
    "vkCreateXlibSurfaceKHR",
    "vkDestroySurfaceKHR",
    "vkCreateDisplayPlaneSurfaceKHR",
    "vkCreateHeadlessSurfaceEXT",
    "vkGetDeviceProcAddr"
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Vulkan Function Hooks
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName)
{
    if (!funcName)
        return NULL;

    // SUPER SPICY -- Unity calls vkGetInstanceProcAddr for ALL functions, including device level ones!
    // From experimentation, unity gets all instance level functions before the device has been created,
    // and all device level ones after device creation despite not needing it for vkGetInstanceProcAddr
    // This patches vkGetInstanceProcAddr to call vkGetDeviceProcAddr instead after the device has been 
    // created. This will probably cause issues with other native plugins and future unity versions!!!!
    /*
    if ( PluginVk::vkDevice != VK_NULL_HANDLE && p_vkGetDeviceProcAddr != nullptr && !instanceLoadedFuncs.contains(std::string_view(funcName)) )
    {
#if defined(_DEBUG)
        std::string msg("Unity Getting Device Level Function through vkGetInstanceProcAddr: ");
        msg.append(funcName);
        PluginState::Log(kUnityLogTypeLog, msg.c_str(), __FILE__, __LINE__);
        //printf(funcName);
#endif
        PFN_vkVoidFunction fnptr = Hook_vkGetDeviceProcAddr(PluginVk::vkDevice, funcName);
        if (fnptr != NULL) return fnptr;
    }
    */
#define INTERCEPT(fn) if (strcmp(funcName, #fn) == 0) return (PFN_vkVoidFunction)&Hook_##fn

#ifndef DUMMY_PLUGIN
    INTERCEPT(vkCreateInstance);
    INTERCEPT(vkCreateDevice);
    INTERCEPT(vkGetDeviceProcAddr); // unity doesn't seem to use vkGetDeviceProcAddr, instead calls vkGetInstanceProcAddr for device functions?
    //INTERCEPT(vkCreateSampler);
    //INTERCEPT(vkCreateGraphicsPipelines);
    //if (!SLZGraphicsConfig::s_Instance->disableSamplerHook)
    { INTERCEPT(vkCreateSampler); }
    INTERCEPT(vkCreateGraphicsPipelines);
#endif

#undef INTERCEPT

    return p_vkGetInstanceProcAddr(device, funcName);
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (!funcName)
        return NULL;

#define INTERCEPT(fn) if (strcmp(funcName, #fn) == 0) { p_##fn = (PFN_##fn)p_vkGetDeviceProcAddr(device, funcName); return (PFN_vkVoidFunction)&Hook_##fn; }

#ifndef DUMMY_PLUGIN
    if (!SLZGraphicsConfig::disableSamplerHook) { INTERCEPT(vkCreateSampler); }
    INTERCEPT(vkCreateGraphicsPipelines);
#endif

#undef INTERCEPT

    return p_vkGetDeviceProcAddr(device, funcName);
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    VkApplicationInfo* newAppInfo = const_cast<VkApplicationInfo*>(pCreateInfo->pApplicationInfo);

    //VkApplicationInfo newAppInfo = {};
    //newAppInfo.sType = pCreateInfo->pApplicationInfo->sType;
    //newAppInfo.pNext = pCreateInfo->pApplicationInfo->pNext;
    //newAppInfo.pApplicationName = pCreateInfo->pApplicationInfo->pApplicationName;
    //newAppInfo.applicationVersion = pCreateInfo->pApplicationInfo->applicationVersion;
    //newAppInfo.pEngineName = pCreateInfo->pApplicationInfo->pEngineName;
    //newAppInfo.engineVersion = pCreateInfo->pApplicationInfo->engineVersion;
    newAppInfo->apiVersion = pCreateInfo->pApplicationInfo->apiVersion < VK_API_VERSION_1_2 ? VK_API_VERSION_1_2 : pCreateInfo->pApplicationInfo->apiVersion;
    
    PluginState::Log(kUnityLogTypeLog,
        std::format("Original Vulkan API requested {}.{}.{}.{} ({})\nNew vulkan API: {}.{}.{}.{} ({})",
            VK_API_VERSION_VARIANT(pCreateInfo->pApplicationInfo->apiVersion),
            VK_API_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion),
            VK_API_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion),
            VK_API_VERSION_PATCH(pCreateInfo->pApplicationInfo->apiVersion),
            pCreateInfo->pApplicationInfo->apiVersion,
            VK_API_VERSION_VARIANT( newAppInfo->apiVersion),
            VK_API_VERSION_MAJOR(   newAppInfo->apiVersion),
            VK_API_VERSION_MINOR(   newAppInfo->apiVersion),
            VK_API_VERSION_PATCH(   newAppInfo->apiVersion),
            newAppInfo->apiVersion
            ).c_str(), __FILE__, __LINE__);

    const VkInstanceCreateInfo* newCreateInfo = pCreateInfo;
    //uint32_t extCount = 0;
    //
    //VkInstanceCreateInfo newCreateInfo = {};
    //newCreateInfo.sType = pCreateInfo->sType;
    //newCreateInfo.pNext = pCreateInfo->pNext;
    //newCreateInfo.flags = pCreateInfo->flags;
    //newCreateInfo.pApplicationInfo = &newAppInfo;
    //newCreateInfo.enabledLayerCount = pCreateInfo->enabledLayerCount;
    //newCreateInfo.ppEnabledLayerNames = pCreateInfo->ppEnabledLayerNames;
    //newCreateInfo.enabledExtensionCount = pCreateInfo->enabledExtensionCount;
    //newCreateInfo.ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames;

    p_vkCreateInstance = (PFN_vkCreateInstance)p_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");

    VkResult result = p_vkCreateInstance(newCreateInfo, pAllocator, pInstance);
    if (result == VK_SUCCESS)
    {
        PluginVk::vkDevice = VK_NULL_HANDLE;
        LoadVulkanAPI(p_vkGetInstanceProcAddr, *pInstance);
    }
    return result;
}

// C++ lacks a nameof operator or a sane way to turn an enum to its string name.
// Do preprocessor bullshit instead. Declare the body of the RequestedVkExtensions enum
// and extNames array using the same macro to ensure all extensions are declared in
// the matching order in both. that way we can use RequestedVkExtensions as an index
// into extNames without having to manually sync the contents of eachother.
// Note that "VK_" is removed from all extension names as the full name is declared as
// a preprocessor macro in the vulkan headers. Use more macros to tack that back on later
#define REQUESTED_VK_EXTS(macro) \
    macro(KHR_fragment_shading_rate) \
    macro(KHR_fragment_shader_barycentric) \
    //macro(NV_shading_rate_image) \
    //macro(EXT_extended_dynamic_state3) \
    //macro(EXT_blend_operation_advanced)

#define ENUM_DEF(x) x,

enum RequestedVkExtensions
{
    REQUESTED_VK_EXTS(ENUM_DEF)

    ExtCount // Keep Last!
};

#undef ENUM_DEF


static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    PluginState::Log(kUnityLogTypeLog, "Hook_vkCreateDevice Called!", __FILE__, __LINE__);

    //Cast the createinfo to an modifiable pointer so we don't have to copy everything. Unsafe asf, but hasn't caused any issues so far...
    VkDeviceCreateInfo* newCreateInfo = const_cast<VkDeviceCreateInfo*>(pCreateInfo);

    PluginVk::s_UnityAllocatorCallbacks = pAllocator;

    // Get all the extensions available for the physicial device
    uint32_t availableDeviceExtCount;
    p_vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtCount, nullptr);
    unique_ptr<VkExtensionProperties[]> availableDeviceExt(new VkExtensionProperties[availableDeviceExtCount]);
    p_vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableDeviceExtCount, availableDeviceExt.get());

    constexpr int extCount = RequestedVkExtensions::ExtCount;

    #define EXT_STR_NAME(f) "VK_" #f ,
    const char* extNames[extCount] =
    {
        REQUESTED_VK_EXTS(EXT_STR_NAME)
    };
    #undef EXT_STR_NAME

    bool extIsAvailable[extCount];
    bool extNotSet[extCount];

    for (int extIdx = 0; extIdx < extCount; extIdx++)
    {
        extIsAvailable[extIdx] = false;
        extNotSet[extIdx] = true;
    }

    int availableExtCount = 0;
    int enableExtCount = 0;

    // Check if each extension is available
    for (uint32_t avIdx = 0; avIdx < availableDeviceExtCount; avIdx++)
    {
        for (int i = 0; i < extCount; i++)
        {
            if (!extIsAvailable[i] && (strcmp(availableDeviceExt[avIdx].extensionName, extNames[i]) == 0))
            {
                extIsAvailable[i] = true;
                availableExtCount++;
            }
        }
    }

    availableDeviceExt.release();



    //  Create a chain of physical device property 2 structs for each extension that has them
    VkPhysicalDeviceProperties2 deviceProps = {};
    deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    VkBaseOutStructure* propertyChainNext = (VkBaseOutStructure*)&deviceProps;

    PluginVk::s_VrsProps = new VkPhysicalDeviceFragmentShadingRatePropertiesKHR;
    memset(PluginVk::s_VrsProps, 0, sizeof(VkPhysicalDeviceFragmentShadingRatePropertiesKHR));
    PluginVk::s_VrsProps->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
    PluginVk::s_VrsProps->pNext = nullptr;

    if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shading_rate])
    {
        propertyChainNext->pNext = (VkBaseOutStructure*)PluginVk::s_VrsProps;
        propertyChainNext = (VkBaseOutStructure*)PluginVk::s_VrsProps;
    }

    VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR physDevBaryProps = {};
    physDevBaryProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR;
    physDevBaryProps.pNext = nullptr;

    if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shader_barycentric])
    {
        propertyChainNext->pNext = (VkBaseOutStructure*)(&physDevBaryProps);
        propertyChainNext = (VkBaseOutStructure*)(&physDevBaryProps);
    }

    p_vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps);

    // Get graphics hardware info

    PluginVk::s_vendorID = deviceProps.properties.vendorID;
    uint32_t driverVersion = deviceProps.properties.driverVersion;
    uint32_t deviceID = deviceProps.properties.deviceID;


    // Check if unity already set any of the available extensions we're trying to add
    uint32_t newExtCount = pCreateInfo->enabledExtensionCount;
    enableExtCount = availableExtCount;
    for (uint32_t i = 0; i < newExtCount; i++)
    {
        for (uint32_t j = 0; j < extCount; j++)
        {
            if (extIsAvailable[j] && extNotSet[j] && (strcmp(pCreateInfo->ppEnabledExtensionNames[i], extNames[j]) == 0))
            {
                extNotSet[j] = false;
                enableExtCount--;
                break;
            }
        }
    }

    // Make a new list of extensions, copying all of unity's and adding the ones we need that haven't been set yet
    newExtCount += enableExtCount;
    unique_ptr<const char* []> newEnabledExtNames(new const char* [newExtCount]);
    memcpy(newEnabledExtNames.get(), pCreateInfo->ppEnabledExtensionNames, pCreateInfo->enabledExtensionCount * sizeof(char*));

    int nextExt = pCreateInfo->enabledExtensionCount;
    for (int i = 0; i < extCount; i++)
    {
        if (extIsAvailable[i] && extNotSet[i])
        {
            newEnabledExtNames[nextExt] = extNames[i];
            nextExt++;
        }
    }

    // set the new extension name list and count on the device create info
    newCreateInfo->enabledExtensionCount = newExtCount;
    newCreateInfo->ppEnabledExtensionNames = newEnabledExtNames.get();

    bool hasKhrShadingRate = false;
    if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shading_rate])
    {
        hasKhrShadingRate = true;
    }

    // Create a chain of physical device FEATURES 2 stucts for each extension that has them

    VkPhysicalDeviceFeatures2 deviceFeatures = {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkBaseOutStructure* featureChainNext = (VkBaseOutStructure*)&deviceFeatures;


    VkPhysicalDeviceFragmentShadingRateFeaturesKHR vrsFeatures = {};
    vrsFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
    if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shading_rate])
    {
        featureChainNext->pNext = (VkBaseOutStructure*)&vrsFeatures;
        featureChainNext = featureChainNext->pNext;
    }

    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR baryFeatures = {};
    baryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
    baryFeatures.pNext = nullptr;

    if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shader_barycentric])
    {
        featureChainNext->pNext = (VkBaseOutStructure*)(&baryFeatures);
        featureChainNext = (VkBaseOutStructure*)(&baryFeatures);
    }


    VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT advBlendOps = {};
    advBlendOps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
    
    



    // Get device features
    p_vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);




    //hasNvShadingRate = hasNvShadingRate && dynState3Features.extendedDynamicState3ShadingRateImageEnable;

    PluginVk::s_SupportsPerPipelineShadingRate = hasKhrShadingRate && vrsFeatures.pipelineFragmentShadingRate;
    PluginVk::s_SupportsLayeredShadingRate = hasKhrShadingRate && PluginVk::s_VrsProps->layeredShadingRateAttachments;

    bool originalAttachmentFragmentShadingRate = vrsFeatures.attachmentFragmentShadingRate;
    
    /*
    if (extIsAvailable[RequestedVkExtensions::EXT_blend_operation_advanced])
    {
        DebugLog::LogWinDbgString(std::format("Vk_EXT_blend_operation_advanced is available, supports coherent ops {}", advBlendOps.advancedBlendCoherentOperations).c_str());
        PluginVk::s_SupportsBlendOpAdv = advBlendOps.advancedBlendCoherentOperations;
    }
    else
    */
    {
       
        PluginVk::s_SupportsBlendOpAdv = false;
    }

    PluginState::Log(kUnityLogTypeLog, std::format("Barycentrics Extension Available {}", baryFeatures.fragmentShaderBarycentric).c_str(), "", 0);

    //if (extIsAvailable[RequestedVkExtensions::KHR_fragment_shader_barycentric])
    //{
    //}
    
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR* defaultFeatures = nullptr;
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR* defaultBaryFeatures = nullptr;
    
    void* pNext = (void*)newCreateInfo->pNext;
    void* previousStruct = (void*)&newCreateInfo;

    bool hasFragmentDensityMap = false;
    bool hasBaryCentrics = false;
    // Scan the original extension features 
    while (pNext)
    {
        previousStruct = pNext;
        VkStructureType pNextType = ((VkBaseOutStructure*)pNext)->sType;
        if (pNextType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR)
        {
            defaultFeatures = (VkPhysicalDeviceFragmentShadingRateFeaturesKHR*)pNext;
        }
        // If fragment density map is enabled, enabling fragment shading rate is FORBIDDEN BY THE SPEC!!!! https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VUID-VkDeviceCreateInfo-fragmentDensityMap-04481
        // However, due to a quirk of the Adreno GPU+driver used by the Quest 2 and 3, the whole pipeline version of fragment shading rate works without issue
        // We cannot guarantee that this will continue to work in future devices/driver versions! Ideally gate this to the Quest 2/Pro/3/3s (need to collect device ids)
        else if (pNextType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT)
        {
            if (((VkPhysicalDeviceFragmentDensityMapFeaturesEXT*)pNext)->fragmentDensityMap)
            {
                hasFragmentDensityMap = true;
                PluginVk::s_SupportsPerPipelineShadingRate = PluginVk::s_vendorID == PCIE_VENDOR_ID_QCOM; // && driverVersion <= latestSupportedQCOMDriver;
                if (!PluginVk::s_SupportsPerPipelineShadingRate)
                {
                    PluginState::Log(kUnityLogTypeError, "SLZ Vk Plugin: HALF RATE SHADING HACK DISABLED!!!! Fragment density map and fragment shading rate cannot be enabled together on this hardware!", "", 0);
                    PluginState::Log(kUnityLogTypeError, "Enabling density map and fragment shading rate is technically forbidden, but they have been tested to actually work together on Adreno hardware", "", 0);
                    PluginState::Log(kUnityLogTypeError, std::format("This vendor id: {:#x}, device id : {:#x}, driver version: {:#x}, (Adreno vendor ID is {:#x})", PluginVk::s_vendorID, deviceID, driverVersion, (uint32_t)PCIE_VENDOR_ID_QCOM).c_str(), "", 0);
                }
                else
                {
                    PluginState::Log(kUnityLogTypeError, "SLZ Vk Plugin: Half rate shading hack was enabled with Fragment Density Map! If crashing or graphical corruption occurs, disable for this device/driver version!", "", 0);
                    PluginState::Log(kUnityLogTypeError, "Enabling density map and fragment shading rate is technically forbidden, but they have been tested to actually work together on Adreno hardware", "", 0);
                    PluginState::Log(kUnityLogTypeError, std::format("This vendor id: {:#x}, device id : {:#x}, driver version: {:#x}, (Adreno vendor ID is {:#x})", PluginVk::s_vendorID, deviceID, driverVersion, (uint32_t)PCIE_VENDOR_ID_QCOM).c_str(), "", 0);
                }
            }
        }

        if (pNextType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR)
        {
            defaultBaryFeatures = (VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR*)pNext;
        }

        pNext = ((VkBaseOutStructure*)pNext)->pNext;
    }

    VkBool32 originalPipelineFragmentShadingRate = vrsFeatures.pipelineFragmentShadingRate;
    if (defaultFeatures)
    {
        originalPipelineFragmentShadingRate = defaultFeatures->pipelineFragmentShadingRate;

        defaultFeatures->pipelineFragmentShadingRate = vrsFeatures.pipelineFragmentShadingRate;
        defaultFeatures->primitiveFragmentShadingRate  = vrsFeatures.primitiveFragmentShadingRate;
        defaultFeatures->attachmentFragmentShadingRate = vrsFeatures.attachmentFragmentShadingRate;

    }
    else if (hasKhrShadingRate)
    {
        ((VkBaseOutStructure*)previousStruct)->pNext = (VkBaseOutStructure*)&vrsFeatures;
        previousStruct = &vrsFeatures;
        defaultFeatures = &vrsFeatures;
    }

    if (!defaultBaryFeatures && extIsAvailable[RequestedVkExtensions::KHR_fragment_shader_barycentric])
    {
        ((VkBaseOutStructure*)previousStruct)->pNext = (VkBaseOutStructure*)&baryFeatures;
        previousStruct = &baryFeatures;
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR* usedFragRateFeatures = defaultFeatures != nullptr ? defaultFeatures : &vrsFeatures;

    if (hasFragmentDensityMap) // Disable primitive and pipeline rates
    {
        usedFragRateFeatures->attachmentFragmentShadingRate = 0;
        usedFragRateFeatures->primitiveFragmentShadingRate = 0;
    }

    VkResult result = p_vkCreateDevice(physicalDevice, newCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS)
    {
        // Don't load device-level functions properly. Unity loads them via the instance, 
        // if other plugins or vulkan layers attempt to hook the same functions using vkGetDeviceProcAddr will
        // remove their hook
        PluginVk::vkDevice = *pDevice;
        LoadVulkanAPIDevice(p_vkGetDeviceProcAddr, *pDevice);

        // After creating the device, reset the pipeline shading rate value. This should hide the fact that we enabled it from unity if the fragment density map was also enabled 
        usedFragRateFeatures->pipelineFragmentShadingRate = originalPipelineFragmentShadingRate; 
    }
    return result;
}

#define SLZ_COARSE_RASTER_FLAG 0.8148f
#define SLZ_COARSE_RASTER_FLAG_G 0.817f
#define SLZ_COARSE_RASTER_FLAG_L 0.813f

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateGraphicsPipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    if (PluginVk::s_SupportsPerPipelineShadingRate && SLZGraphicsConfig::allow2x2ShadingHack)
    {   
        int numNeedVRS = 0;
        char* needsVRS;
        bool heapAllocVRSFlags = createInfoCount > 32;
        if (heapAllocVRSFlags)
        {
            needsVRS = (char*)malloc(sizeof(char) * createInfoCount);
        }
        else
        {
            needsVRS = (char*)_malloca(sizeof(char) * createInfoCount);
        }

        //VkGraphicsPipelineCreateInfo* pipelineEdit = const_cast<VkGraphicsPipelineCreateInfo*>(pCreateInfos);
        for (int i = 0; i < createInfoCount; i++)
        {
            needsVRS[i] = 0;
            if (pCreateInfos[i].pRasterizationState->depthBiasEnable)
            {

                // Look at the depth-bias slope factor, and if it ends with 0.0008148 then rasterize at quarter rate
                float depthBias = pCreateInfos[i].pRasterizationState->depthBiasSlopeFactor;
                float flag = 1000.0f * fabsf(depthBias);
                flag = flag - floorf(flag);
                if (flag >= SLZ_COARSE_RASTER_FLAG_L && flag <= SLZ_COARSE_RASTER_FLAG_G)
                {
                    needsVRS[i] = 1;
                    numNeedVRS += 1;
                }
            }
        }

        VkPipelineFragmentShadingRateStateCreateInfoKHR* vrsStructs;
        bool heapAllocVRSStructs = (numNeedVRS * sizeof(VkPipelineFragmentShadingRateStateCreateInfoKHR)) > 64;
        if (heapAllocVRSStructs)
        {
            vrsStructs = (VkPipelineFragmentShadingRateStateCreateInfoKHR*)
                malloc(sizeof(VkPipelineFragmentShadingRateStateCreateInfoKHR) * numNeedVRS);
        }
        else
        {
            vrsStructs = (VkPipelineFragmentShadingRateStateCreateInfoKHR*)
                _malloca(sizeof(VkPipelineFragmentShadingRateStateCreateInfoKHR) * numNeedVRS);
        }

        for (int i = 0; i < createInfoCount; i++)
        {
            if (needsVRS[i])
            {
                vrsStructs[i] = PluginVk::s_pipelineRate2x2;
                float depthBias = pCreateInfos[i].pRasterizationState->depthBiasSlopeFactor;
                //reset bias to 0 if bias is +/- 0.0008148
                if (fabsf(depthBias) <= (SLZ_COARSE_RASTER_FLAG_G * 0.001f))
                {
                    VkPipelineRasterizationStateCreateInfo* editRasterState = const_cast<VkPipelineRasterizationStateCreateInfo*>(pCreateInfos[i].pRasterizationState);
                    editRasterState->depthBiasSlopeFactor = 0.0f;
                    if (editRasterState->depthBiasConstantFactor == 0) editRasterState->depthBiasEnable = false;
                }
                bool otherRate = false;
                const VkBaseOutStructure* next = (const VkBaseOutStructure*)&pCreateInfos[i];
                while (next->pNext != NULL)
                {
                    next = (const VkBaseOutStructure*)(next->pNext);
                    //Make sure that somebody else didn't already add a fragment shading rate struct to the chain
                    if (next->sType == VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR)
                    {

                        otherRate = true;

                        VkPipelineFragmentShadingRateStateCreateInfoKHR* editRate = (VkPipelineFragmentShadingRateStateCreateInfoKHR*)const_cast<VkBaseOutStructure*>(next);
                        editRate->fragmentSize = vrsStructs[i].fragmentSize;

                        break;
                    }
                    
                }

                if (!otherRate)
                {
                    vrsStructs[i].pNext = pCreateInfos[i].pNext;
                    VkGraphicsPipelineCreateInfo* editCreateInfo = const_cast<VkGraphicsPipelineCreateInfo*>(&pCreateInfos[i]);
                    editCreateInfo->pNext = &vrsStructs[i];
                }
            }
        }

        if (heapAllocVRSFlags)
        {
            free(needsVRS);
        }
        else
        {
            _freea(needsVRS);
        }

        VkResult createPipelinesResult = p_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);

        if (heapAllocVRSStructs)
        {
            free(vrsStructs);
        }
        else
        {
            _freea(vrsStructs);
        }

        return createPipelinesResult;
    }

    VkResult createPipelinesResult = p_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return createPipelinesResult;
}



static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    VkSamplerCreateInfo* eCreateInfo = const_cast<VkSamplerCreateInfo*>(pCreateInfo);
    if (eCreateInfo->anisotropyEnable)
    {
        float maxAnisotropy = SLZGraphicsConfig::s_Instance->maxAnisotropy;
        if (maxAnisotropy >= 0.0f && maxAnisotropy < 16.0f)
        {
            eCreateInfo->maxAnisotropy = fminf(eCreateInfo->maxAnisotropy, maxAnisotropy);
        }
        
        int maxAniso = (int)(eCreateInfo->maxAnisotropy);
        int anisoMipBiasIdx = min(max(maxAniso, 0), 16);
        float anisoMipBias = 0.0f;
        // Nvidia's mip biases in DX11 when anisotropic filtering of 2x, 4x, 6x, >=8x are applied
        if (PluginVk::s_vendorID == PCIE_VENDOR_ID_NVIDIA)
        {
            const float anisoMipBiasLUT[] = { 0, 0, -0.1667, -0.1667, -0.2125, -0.2125, -0.2125, -0.2125, -0.25, -0.25, -0.25, -0.25, -0.25, -0.25, -0.25, -0.25, -0.25 };
            anisoMipBias = anisoMipBiasLUT[anisoMipBiasIdx];
        }

        eCreateInfo->mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        eCreateInfo->mipLodBias = eCreateInfo->mipLodBias + anisoMipBias;
    }

    return p_vkCreateSampler(device, eCreateInfo, pAllocator, pSampler);
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2(
    VkDevice device,
    const VkRenderPassCreateInfo2* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass
    )
{
    bool isVR = pCreateInfo->pSubpasses[0].viewMask == 3;
    if (isVR)
    {

    }
}


extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API PrintFoveatedPointer(void* ptr)
{
    if (ptr == nullptr)
    {
        PluginState::Log(kUnityLogTypeLog,"Foveated struct Pointer was NULL", __FILE__, __LINE__);
        return;
    }
    UnityFoveatedInfo* info = (UnityFoveatedInfo*)ptr;
    string message = "Foveated Data\n";
    message += "\nstruct size " + to_string(info->size);
    message += "\nunknown1 " + to_string(info->unknown1);
    message += "\nunknown2 " + to_string(info->unknown2);
    message += "\nviews " + to_string(info->depth);
    message += "\nwidth " + to_string(info->width);
    message += "\nheight " + to_string(info->height);
    message += "\nunknown3 " + to_string(info->unknown3);
    message += "\nunknown4 " + to_string(info->unknown4);
    message += "\nunknown5 " + to_string(info->unknown5);
    message += "\nunknown6 " + to_string(info->unknown6);
    message += "\nunknown7 " + to_string(info->unknown7);
    message += "\nunknown8 " + to_string((size_t)info->colorBufferPointer);
    PluginState::Log(kUnityLogTypeLog, message.data(), __FILE__, __LINE__);
}

uint32_t FindMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps;
    p_vkGetPhysicalDeviceMemoryProperties(PluginVk::s_UnityInstanceVk.physicalDevice, &memProps);
    for (int i = 0; i < memProps.memoryTypeCount; i++) {
        if (memoryTypeBits & (1 << i)) {
            return i;
        }
    }
    return -1;
}

void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) 
{
    VkDevice device = PluginVk::s_UnityInstanceVk.device;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (p_vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    p_vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (p_vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    p_vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

class TempCmdBuffer
{
public:
    VkCommandBuffer cmd;
    TempCmdBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = PluginVk::instance->gfxCommandPool;
        allocInfo.commandBufferCount = 1;

       
        p_vkAllocateCommandBuffers(PluginVk::s_UnityInstanceVk.device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        p_vkBeginCommandBuffer(cmd, &beginInfo);
    }

    ~TempCmdBuffer()
    {
        p_vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        p_vkQueueSubmit(PluginVk::s_UnityInstanceVk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        p_vkQueueWaitIdle(PluginVk::s_UnityInstanceVk.graphicsQueue);

        p_vkFreeCommandBuffers(PluginVk::s_UnityInstanceVk.device, PluginVk::instance->gfxCommandPool, 1, &cmd);
    }
};

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
{
    TempCmdBuffer tmpCmd = TempCmdBuffer();
    VkCommandBuffer cmd = tmpCmd.cmd;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    p_vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

extern "C" UNITY_INTERFACE_EXPORT  void* UNITY_INTERFACE_API CreateTextureFromData(void* ptr, unsigned int width, unsigned int height, unsigned int format, unsigned long byteCount)
{
    if (PluginVk::s_UnityInstanceVk.device == VK_NULL_HANDLE)
    {
        return NULL;
    }

    VkBuffer buffer;
    VkDeviceMemory bufferMem;

    CreateBuffer(byteCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMem);

    void* texData;
    p_vkMapMemory(PluginVk::s_UnityInstanceVk.device, bufferMem, 0, byteCount, 0, &texData);
    memcpy(texData, ptr, byteCount);
    p_vkUnmapMemory(PluginVk::s_UnityInstanceVk.device, bufferMem);

    VkFormat imgFormat = (VkFormat)format;
    VkImage* texture = (VkImage*)malloc(sizeof(VkImage));
    VkDeviceMemory textureMemory = {};
    VkImageCreateInfo info =
    {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,
        VK_IMAGE_TYPE_2D,
        imgFormat,
        {width, height, 1},
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (p_vkCreateImage(PluginVk::s_UnityInstanceVk.device, &info, nullptr, texture) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memReq;
    p_vkGetImageMemoryRequirements(PluginVk::s_UnityInstanceVk.device, *texture, &memReq);
    uint32_t memType = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memType;

    p_vkBindImageMemory(PluginVk::s_UnityInstanceVk.device, *texture, textureMemory, 0);

    if (p_vkAllocateMemory(PluginVk::s_UnityInstanceVk.device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    transitionImageLayout(*texture, imgFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    {
        TempCmdBuffer buf = TempCmdBuffer();
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;

        copyRegion.imageOffset = { 0, 0, 0 };
        copyRegion.imageExtent = {
            width,
            height,
            1
        };

        p_vkCmdCopyBufferToImage(buf.cmd, buffer, *texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    }
    transitionImageLayout(*texture, imgFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return texture;
}
