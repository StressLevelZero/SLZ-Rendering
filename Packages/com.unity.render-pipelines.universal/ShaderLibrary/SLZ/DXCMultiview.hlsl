
// Disgustingly cursed hack to make unity compile multiview shaders correctly with DXC for vulkan. 
//    
// Multiview does not exist in D3D11 (excluding a hacky nvidia d3d11 extension) or HLSL Shader Model 5.0.
// The way Unity got their old FXC->HLSLCC->GLSL compiler chain to output the GLSL value for the view index that had no equivalent in HLSL was to modify hlslcc to search 
// for a cbuffer with a magic name (OVR_Multiview) and replace it with references to the gl_ViewID_OVR builtin during translation to glsl. 
//
// Since DXC does not go through hlslcc, the buffer isn't replaced and the shader uses the dummy buffer value as the eye index. However, D3D12 SM 6.1 added multiview
// support with SV_ViewID being the HLSL semantic for the view index. Thus, it should be possible to get DXC to compile multiview shaders correctly; we just need to
// use SV_ViewID instead of the dummy cbuffer value. Ideally, we would add SV_ViewID as a separate input to each stage as it is accessable from the vertex, tesselation,
// and fragment stages. That would require re-writing every shader, so instead we can try to override the existing stereo and instancing macros that are already standard
// in every shader's vertex input and output structs. The only macro in the vertex input struct is UNITY_VERTEX_INPUT_INSTANCE_ID. We can't put SV_ViewID in that as it
// also gets put into the vertex output struct, and SV_ViewID cannot be written to. Instead, we redefine the POSITION semantic to have ViewIndex semantic appended after it.
// This abuses the fact that HLSL allows every semantic to be numbered, using POSITION0 instead of POSITION to prevent recursion in the macro.
//
// As a final fuck you from Unity, it turns out we can't use SV_ViewID. It was introduced in shader model 6.1. Unity explicitly asks DXC for SM 6.0 unless one of a small list
// of pre-approved features are requested that happen to come from a later shader model. However, DXC comes to our rescue and provides us a method to inline SPIR-V instructions 
// directly. This allows us to request the vulkan multiview extension and override a fake vertex input with the ViewIndex builtin.
//
// Additional changes were made to core/ShaderLibrary/UnityInstancing and universal/ShaderLibrary/UnityInput to modify the instancing and stereo macros to pass and store the new ViewIndex correctly


#if defined(STEREO_MULTIVIEW_ON) && defined(UNITY_COMPILER_DXC) && defined(SHADER_API_VULKAN)
    #define DXC_MULTIVIEW
    [[vk::ext_capability(/*MultiView*/ 4439)]]
    [[vk::ext_extension("SPV_KHR_multiview")]]
    void RequestMultiview() {}
    #ifdef UNITY_INSTANCING_INCLUDED
        #error UnityInstancing included before DXCMultiview
    #endif


    #define POSITION POSITION0; [[vk::ext_decorate(/*Builtin*/11, /*ViewIndex*/4440)]] uint stereoTargetEyeIndexAsBlendIdx0 : VIEWIDX
    
    #define INPUT_VIEWINDEX , [[vk::ext_decorate(/*Builtin*/11, /*ViewIndex*/4440)]] uint stereoTargetEyeIndexAsBlendIdx0 : VIEWIDX

#else
    #define INPUT_VIEWINDEX
#endif
