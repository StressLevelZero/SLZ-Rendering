#define LEGACY_SHADER

// Fog

#if defined(ASE_FOG)
    #if defined(FOG_LINEAR)
        #undef FOG_LINEAR
        #define FOG_LINEAR 1
    #else
        #define FOG_LINEAR 0
    #endif
    
    #if defined(FOG_EXP)
        #undef FOG_EXP
        #define FOG_EXP 1
    #else
        #define FOG_EXP 0
    #endif
    
    #if defined(FOG_EXP2)
        #undef FOG_EXP2
        #define FOG_EXP2 1
    #else
        #define FOG_EXP2 0
    #endif
#endif

#if defined(LIGHTMAP_ON) && defined(DYNAMICLIGHTMAP_ON)
    #define SAMPLE_GI_DIR(lmUVOrSH, dynUV, vertexSH, normalWS, smoothness, viewDir) SAMPLE_GI(lmUVOrSH, dynUV, vertexSH, normalWS)
#elif defined(DYNAMICLIGHTMAP_ON)
    #define SAMPLE_GI_DIR(lmUVOrSH, dynUV, vertexSH, normalWS) SAMPLE_GI(lmUVOrSH, dynUV, vertexSH, normalWS)
#elif defined(LIGHTMAP_ON)
    #define SAMPLE_GI_DIR(lmUVOrSH, vertexSH, normalWS, smoothness, viewDir) SAMPLE_GI(lmUVOrSH, vertexSH, normalWS)
#else
    #define SAMPLE_GI_DIR(lmUVOrSH, vertexSH, normalWS, smoothness, viewDir) SAMPLE_GI(lmUVOrSH, vertexSH, normalWS)
#endif

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"

#define DecodeHDREnvironment2 DecodeHDREnvironment

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ShaderVariablesFunctions.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/VolumetricCore.hlsl"

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/BRDF.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZExtentions.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/SLZShadows.hlsl"

half3 MixFog(real3 fragColor, float3 viewDirectionWS, real fogFactor) { return half4(1,0.75,1,1) * fragColor; }
half4 MixFogColorSurf(half4 fragColor, half3 viewDirectionWS, half fogFactor, int surface) { return half4(1,0.75,1,1) * fragColor; }