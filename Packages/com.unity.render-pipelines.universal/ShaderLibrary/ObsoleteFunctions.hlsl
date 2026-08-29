#define LEGACY_SHADER

// Fog

#if !defined(USE_DYNAMIC_BRANCH_FOG_KEYWORD)

#define HASH #
#define BYPASS_UNITY_IFNDEF(hash, value) hash if !defined ( FOG_##value ) 
#define BYPASS_UNITY_DEFINE(hash, value, value2) hash define FOG_##value value2 
#define BYPASS_UNITY_ENDIF(hash) hash endif 


#if !defined(FOG_LINEAR_KEYWORD_DECLARED)
BYPASS_UNITY_DEFINE(HASH, LINEAR, 0)
#endif


#if !defined(FOG_EXP_KEYWORD_DECLARED)
BYPASS_UNITY_DEFINE(HASH, EXP, 0)
#endif

#if !defined(FOG_EXP2_KEYWORD_DECLARED)
BYPASS_UNITY_DEFINE(HASH, EXP2, 0)
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


#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ShaderVariablesFunctions.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/VolumetricCore.hlsl"

half3 MixFogColor(half3 fragColor, half3 fogColor, float3 viewDirectionWS, half fogFactor)
{
    if (FOG_LINEAR || FOG_EXP || FOG_EXP2)
    {
        half fogIntensity = ComputeFogIntensity((half)fogFactor);
        min16float3 mipFog = MipFog(viewDirectionWS, fogFactor, 7);
        fragColor = lerp(mipFog, fragColor, fogIntensity);
    }
    return fragColor;
}

half3 MixFog(min16float3 fragColor, float3 viewDirectionWS, min16float fogFactor)
{
    return  (half4(MixFogColor(fragColor, unity_FogColor.rgb, viewDirectionWS, fogFactor), 1)).rgb;
}

half4 MixFogColorSurf(half4 fragColor, half3 viewDirectionWS, half fogFactor, int surface)
{
    if (FOG_LINEAR || FOG_EXP || FOG_EXP2)
    {
        half fogIntensity = ComputeFogIntensity(fogFactor);
    
        half3 mipFog = MipFog(viewDirectionWS, fogFactor, 7 );
        if (surface == 1) // 1 = Transparent, which is actually alpha premultiplied.
        {
            mipFog *= fragColor.a;
        }
        fragColor.rgb = lerp(mipFog, fragColor.rgb, fogIntensity);
    }
    return fragColor;
}

half4 MixFogSurf(half4 fragColor, half3 viewDirectionWS, half fogFactor, int surface)
{
    return  MixFogColorSurf(fragColor, viewDirectionWS, fogFactor, surface);
}


