#if !defined(NORMALS_BUFFER_INCLUDED)
#define NORMALS_BUFFER_INCLUDED
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/PackNormalsTexture.hlsl"

#if !defined(_SLZ_NORMALS_OCT_R8G8_KEYWORD_DECLARED)
    #if !defined(_SLZ_NORMALS_OCT_R8G8)
        #define _SLZ_NORMALS_OCT_R8G8 0
    #elif DEFINED_NONZERO(_SLZ_NORMALS_OCT_R8G8)
        #undef _SLZ_NORMALS_OCT_R8G8
        #define _SLZ_NORMALS_OCT_R8G8 1
    #endif
#endif

namespace SLZ
{
    half4 PackNormalsToGBuffer(half3 normalWS)
    {
        if (_SLZ_NORMALS_OCT_R8G8)
        {
            half2 octNormalWS = PackNormalOctQuadEncode(normalWS);
            half2 remappedOctNormalWS = saturate(octNormalWS * half(0.5) + half(0.5)); 
            return half4(remappedOctNormalWS, 0.0, 0.0);
        }
        else
        {
           return half4(PackNormalWSToTexture(normalWS), 0.0);
        }
    }

    half3 UnpackNormalsFromGBuffer(half4 bufferValue)
    {
        if (_SLZ_NORMALS_OCT_R8G8)
        {
            half2 octNormalWS = 2.0 * bufferValue.rg - 1.0; 
            half3 normalWS = UnpackNormalOctQuadEncode(octNormalWS);
            return normalWS;
        }
        else
        {
            return UnpackNormalWSFromTexture(bufferValue.rgb);
        }
    }
}

#endif
