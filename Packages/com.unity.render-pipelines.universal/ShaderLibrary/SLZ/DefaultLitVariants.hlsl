#if !defined(DEFAULT_LIT_VARIANTS_INCLUDED)
#define DEFAULT_LIT_VARIANTS_INCLUDED

/// INSTRUCTIONS -------------------------------------------------------------

// Just after including the urp Core.hlsl, define the configuration options
// listed below and use #include_with_pragmas to include this file.
// Additionally, paste the following block just after this file. Instancing
// options must be in the main shader file:

/*
#if R_USE_RENDER_LAYERS > 0
#pragma instancing_options renderinglayer
#endif
*/

/// END INSTRUCTIONS ---------------------------------------------------------

/// CONFIGURATION ------------------------------------------------------------

/* configuration defines, define these before including this file!
 * Define each keyword to 0 to disable, 1 to enable a multi-compile,
 * or >=2 to define it as always on for options that support it.
 * Disable or set to permanently on as many options as you can,
 * multi-compiles exponentially increase your compilation time even if
 * the keywords get stripped! Use SHADER_API_MOBILE keyword to change
 * settings for android separately
 * Example config:


#define R_FOG                       1   // 0 - Off, 1 - Dynamic Branch
#define R_ADAPTIVE_PROBE_VOLUMES    1   // 0 - Off, 1 - multi-compile L1 and L2, 2 - L1 always on, 3 - l1+l2 always on
#define R_LIGHTMAP_VARIANTS         0
#define R_DYNAMIC_LIGHTMAPS         0
#define R_LIGHT_LAYERS              0
#define R_USE_RENDERING_LAYERS      0
#define R_INSTANCING                0
#define R_DOTS_INSTANCING           0 
#define R_DECAL_BUFFER              0

*/

/// END CONFIGURATION --------------------------------------------------------

#if !defined(R_ADDITIONAL_LIGHTS_FRAG)
    #if defined(SHADER_API_MOBILE)
        #define R_ADDITIONAL_LIGHTS_FRAG 0
    #else
        #define R_ADDITIONAL_LIGHTS_FRAG 2
    #endif
#endif

#if !defined(R_ADDITIONAL_LIGHTS_VTX)
    #if defined(SHADER_API_MOBILE)
        #define R_ADDITIONAL_LIGHTS_VTX 2
    #else
        #define R_ADDITIONAL_LIGHTS_VTX 0
    #endif
#endif

#if !defined(R_FORWARD_PLUS)
    #if defined(SHADER_API_MOBILE)
        #define R_FORWARD_PLUS 0
    #else
        #define R_FORWARD_PLUS 2
    #endif
#endif

#if !defined(R_LIGHT_LAYERS)
    #define R_LIGHT_LAYERS 0
#endif

#if !defined(R_SCREEN_SPACE_GI)
    #define R_SCREEN_SPACE_GI 0
#endif

#if !defined(R_ADAPTIVE_PROBE_VOLUMES)
    #define R_ADAPTIVE_PROBE_VOLUMES 0
#endif

#if !defined(R_LIGHTMAP_VARIANTS)
    #error R_LIGHTMAP_VARIANTS must be defined as either 0 (Off) or 1 (Multi-Compile)
#endif

#if !defined(R_DYNAMIC_LIGHTMAPS)
    #define R_DYNAMIC_LIGHTMAPS 0
#endif

#if !defined(R_LIGHTMAP_BICUBIC)
    #if defined(SHADER_API_MOBILE)
        #define R_LIGHTMAP_BICUBIC 0
    #else
        #define R_LIGHTMAP_BICUBIC 2
    #endif
#endif

#if !defined(R_DOTS_INSTANCING)
    #if defined(SHADER_API_MOBILE)
        #define R_DOTS_INSTANCING 0
    #else
        #define R_DOTS_INSTANCING 1
    #endif
#endif

#if !defined(R_USE_RENDERING_LAYERS)
    #define R_USE_RENDERING_LAYERS 0
#endif

#if !defined(R_DECAL_BUFFER)
    #define R_DECAL_BUFFER 0
#endif

#if !defined(R_FOG)
     #error R_FOG must be defined as either 0 (Off) or 1 (Dynamic Branch)
#endif

#if !defined(R_INSTANCING)
     #error R_INSTANCING must be defined as either 0 (Off) or 1 (On)
#endif

#if !defined(R_SCREEN_SPACE_OCCLUSION)
    #if defined(SHADER_API_MOBILE)
        #define R_SCREEN_SPACE_OCCLUSION 0
    #else
        #define R_SCREEN_SPACE_OCCLUSION 1
    #endif
#endif

/// END CONFIGURATION --------------------------------------------------


#if R_FOG
    // Always use dynamic branch fog, costs basically nothing and removes up to three keywords

    #define USE_DYNAMIC_BRANCH_FOG_KEYWORD 1
    #if defined(FOG_INCLUDED)
    #error NI
    #endif
    #if !defined(USE_DYNAMIC_BRANCH_FOG_KEYWORD)
    #error NG
    #endif
    #include_with_pragmas "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Fog.hlsl"
#endif


///-------------------------------------------------------------------------
/// Lights
///-------------------------------------------------------------------------


/// Forward+
#if R_FORWARD_PLUS == 1
    #pragma multi_compile _ _CLUSTER_LIGHT_LOOP
    #pragma multi_compile_fragment _ _REFLECTION_PROBE_ATLAS
#elif R_FORWARD_PLUS == 2
#define _CLUSTER_LIGHT_LOOP 1
#define _REFLECTION_PROBE_ATLAS 1
#endif


// soft shadows. Explicit low/med/high keywords unnecessary, the unqualified _SHADOWS_SOFT does a dynamic branch on the quality
#pragma multi_compile_fragment _ _SHADOWS_SOFT //_SHADOWS_SOFT_LOW _SHADOWS_SOFT_MEDIUM _SHADOWS_SOFT_HIGH

// Always use more than 1 cascade if there's shadows. Not worth adding another 
// keyword for the 1 cascade case! This needs to be enforced in the project's settings
#pragma multi_compile _ _MAIN_LIGHT_SHADOWS _MAIN_LIGHT_SHADOWS_CASCADE

#pragma multi_compile _ SHADOWS_SHADOWMASK
#if R_ADDITIONAL_LIGHTS_FRAG
#pragma multi_compile_fragment _ _LIGHT_COOKIES
#endif

#if R_ADDITIONAL_LIGHTS_FRAG
#pragma multi_compile_fragment _ _LIGHT_COOKIES
#endif

#if R_ADDITIONAL_LIGHTS_FRAG == 1
    #pragma multi_compile _ _ADDITIONAL_LIGHTS
    #pragma multi_compile_fragment _ _ADDITIONAL_LIGHT_SHADOWS
#elif R_ADDITIONAL_LIGHTS_FRAG == 2
#define  _ADDITIONAL_LIGHTS 1
    #pragma multi_compile_fragment _ _ADDITIONAL_LIGHT_SHADOWS
#endif

#if R_ADDITIONAL_LIGHTS_VTX == 1
    #pragma multi_compile _ _ADDITIONAL_LIGHTS_VERTEX
#elif R_ADDITIONAL_LIGHTS_FRAG == 2
#define _ADDITIONAL_LIGHTS_VERTEX 1
#endif

#if R_LIGHT_LAYERS == 1
    #pragma multi_compile _ _LIGHT_LAYERS
#elif R_LIGHT_LAYERS == 2
#define _LIGHT_LAYERS 1
#endif


// box projection is cheap enough that it isn't worth a keyword ever
#define _REFLECTION_PROBE_BOX_PROJECTION 1
#define _REFLECTION_PROBE_ROTATION 1

#if !defined(SHADER_API_MOBILE)
    #define _REFLECTION_PROBE_BLENDING 1
#endif

#if R_SCREEN_SPACE_OCCLUSION
    #pragma dynamic_branch _SCREEN_SPACE_OCCLUSION
#endif

#if R_SCREEN_SPACE_GI
    #pragma multi_compile_fragment _ _SCREEN_SPACE_IRRADIANCE
#endif

#if R_DECAL_BUFFER
    #pragma multi_compile_fragment _ _DBUFFER_MRT1 _DBUFFER_MRT2 _DBUFFER_MRT3
#endif

#include_with_pragmas "Packages/com.unity.render-pipelines.core/ShaderLibrary/FoveatedRenderingKeywords.hlsl"

#pragma multi_compile_fragment _ DEBUG_DISPLAY

//-------------------------------------------------------------------------
// GPU Instancing
//-------------------------------------------------------------------------

#if R_ADAPTIVE_PROBE_VOLUMES == 1
    #include_with_pragmas "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ProbeVolumeVariants.hlsl"
#elif R_ADAPTIVE_PROBE_VOLUMES == 2
#define PROBE_VOLUMES_L1 1 
#elif R_ADAPTIVE_PROBE_VOLUMES == 3
#define PROBE_VOLUMES_L2 1
#endif

//-------------------------------------------------------------------------
// GPU Instancing
//-------------------------------------------------------------------------

#if R_INSTANCING
    #pragma multi_compile_instancing
#endif

// Fix for DXC. Unity uses vulkan spec constant to set instancing buffer array size. DXC doesn't allow spec constants to be used as array lengths though.
// Also unity relies on the old hlslCC pipeline to modify the code during translation to replace the original array length (2) with the spec constant.
// On PC, the nvidia drivers don't seem to care that the array length is 2. The whole array is still bound and the shader recieves the correct value for 
// out of bounds indices. Mobile seems to only copy two elements and gives garbage values for OOB indicies.
#if defined(SHADER_API_MOBILE) && defined(UNITY_COMPILER_DXC) && (R_DOTS_INSTANCING || R_INSTANCING)
#pragma instancing_options maxcount:128 forcemaxcount:128
#endif

#if R_DOTS_INSTANCING
    #include_with_pragmas "Packages/com.unity.render-pipelines.universal/ShaderLibrary/DOTS.hlsl"
#endif


#if R_USE_RENDERING_LAYERS
    #include_with_pragmas "Packages/com.unity.render-pipelines.universal/ShaderLibrary/RenderingLayers.hlsl"
#endif

#if R_LIGHTMAP_VARIANTS
    #pragma multi_compile _ LIGHTMAP_ON
    #pragma multi_compile _ DIRLIGHTMAP_COMBINED
    #pragma multi_compile _ LIGHTMAP_SHADOW_MIXING

    #if R_DYNAMIC_LIGHTMAPS
        #pragma multi_compile _ DYNAMICLIGHTMAP_ON
    #endif

    //#pragma multi_compile _ USE_LEGACY_LIGHTMAPS
    #if R_LIGHTMAP_BICUBIC == 1
        #pragma multi_compile_fragment _ LIGHTMAP_BICUBIC_SAMPLING
    #elif R_LIGHTMAP_BICUBIC == 2
        #define LIGHTMAP_BICUBIC_SAMPLING 1
    #endif
#endif

#endif // DEFAULT_LIT_VARIANTS_INCLUDED