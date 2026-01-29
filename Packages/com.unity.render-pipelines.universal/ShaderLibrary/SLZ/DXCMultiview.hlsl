
// Disgustingly cursed hack to make unity compile multiview shaders correctly with DXC for vulkan. 
//    
// Multiview does not exist in D3D11 (excluding a hacky nvidia d3d11 extension) or HLSL Shader Model 5.0.
// The way Unity got it to work with the default FXC->HLSLCC->GLSL compiler chain was to modify hlslcc to search 
// for a cbuffer with a magic name (OVR_Multiview) and replace references to it with the gl_ViewID_OVR builtin during translation to glsl. 
//
// Since DXC does not go through hlslcc, the buffer isn't replaced and the shader uses the dummy buffer value as the eye index. Technically, D3D12 SM 6.1 added multiview
// support so it ought to be trivial to just modify the multi-view macros to use the new SV_ViewID semantic. Unfortunately, unity only instructs the compiler to use SM 6.0 
// so SV_ViewID isn't available. However, DXC comes to our rescue and provides us a method to inline raw SPIR-V instructions directly. This allows us to request the vulkan 
// multiview extension and override a fake vertex input with the ViewIndex builtin.
//
// Additionally, the way the stereo macros are written does not add any additional semantics to the vertex input. Single pass instanced just uses the instancing ID, and 
// multiview used a constant buffer. So we don't have a straight-forward way to inject it into every shader's vertex struct. As a work-around, we redefine the POSITION
// semantic to POSITION0 with the view index appended after it. This abuses the fact that HLSL allows every semantic to be numbered, using POSITION0 instead of POSITION
// to prevent possible recursion.
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
