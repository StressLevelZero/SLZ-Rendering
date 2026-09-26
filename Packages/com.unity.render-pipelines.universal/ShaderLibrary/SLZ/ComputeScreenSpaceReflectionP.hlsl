#ifndef PEMA_SSR_INCLUDED
#define PEMA_SSR_INCLUDED

// Includes
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ComputeScreenSpaceReflection.hlsl"

// SSR implementation based on Pema's implementation. Instead of computing the full SSR color in a deferred pass,
// only compute/store the reflected ray length and create a blurred pyramid of this texture to approximate roughness. 
// The forward pass will use this ray length to determine where to sample the screen color texture.
// This avoids needing to have a smoothness gbuffer, and does not need stochastic ray directions to diffuse object silhouettes.
float ComputeSSRPema(Varyings input) : SV_Target
{
    UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(input);

    float2 positionNDC = input.texcoord;
    float2 positionSS = input.positionCS.xy;
    float deviceDepth = LoadSceneDepth(uint2(positionSS) * _Downsample).r;


    // Calculate ray origin and direction in world space
    float3 normalWS = SampleSceneNormals(positionNDC);
    float3 positionWS = ComputeWorldSpacePosition(positionNDC, deviceDepth, _CameraInverseViewProjections[unity_eyeIndex]);
    float3 positionToCamWS = GetWorldSpaceNormalizeViewDir(positionWS);
    float3 rayDirWS = reflect(-positionToCamWS, normalWS);

    // Apply normal bias with the magnitude dependent on the distance from the camera.
    #ifdef _HIZ_TRACE
    float3 camPosWS = GetCurrentViewPosition();
    positionWS = camPosWS + (positionWS - camPosWS) * (1 - 0.001 * rcp(max(dot(normalWS, positionToCamWS), FLT_EPS)));
    deviceDepth = ComputeNormalizedDeviceCoordinatesWithZ(positionWS, _CameraViewProjections[unity_eyeIndex]).z;
    #endif

    // Transform ray origin and direction to view space.
    float3 positionVS = mul(_CameraViews[unity_eyeIndex], float4(positionWS, 1)).xyz;
    float3 rayDirVS = SafeNormalize(mul(_CameraViews[unity_eyeIndex], float4(rayDirWS, 0)).xyz);

    // Calculate ray end position in view space and screen space
    float rayLength = 1;

    #ifndef _HIZ_TRACE
    // Clamp ray length such that the end point is in front of the camera.
    // Not needed for Hi-Z path as there is no end point, only a direction.
    rayLength = rayDirVS.z > 0 ? min(GetMaxRayLength(), -positionVS.z / rayDirVS.z * 0.999) : GetMaxRayLength();
    #endif

    float3 endPosVS = positionVS + rayDirVS * rayLength;
    float3 startPosNDC = float3(positionNDC, deviceDepth);
    float3 endPosNDC = ComputeNormalizedDeviceCoordinatesWithZ(endPosVS, _CameraProjections[unity_eyeIndex]);

    #ifndef _HIZ_TRACE
    // Clamp ray length such that the end point is within the view frustum.
    // Not needed for Hi-Z path as there is no end point, only a direction.
    float3 rayDeltaNDC = endPosNDC - startPosNDC;
    float rayLengthNDC = length(rayDeltaNDC);
    float3 rayDirNDC = rayDeltaNDC * rcp(rayLengthNDC);
    float3 maxDistanceNDC = rayDirNDC >= 0 ? (1 - startPosNDC) / rayDirNDC : -startPosNDC / rayDirNDC;
    endPosNDC = startPosNDC + rayDirNDC * min(rayLengthNDC, min(maxDistanceNDC.x, min(maxDistanceNDC.y, maxDistanceNDC.z)));
    #endif

    float4 screenSizeWithInverse = _BlitTexture_TexelSize;
    float2 endPosSS = endPosNDC.xy * screenSizeWithInverse.zw;

    float3 rayHitPosNDC;
    int iterCount;
    bool hit;
    #ifdef _HIZ_TRACE
    hit = TraceScreenSpaceRayHiZ(positionSS, deviceDepth, endPosSS.xy, endPosNDC.z, screenSizeWithInverse.zw, rayHitPosNDC, iterCount);
    #else
    hit = TraceScreenSpaceRay(positionSS, deviceDepth, endPosSS.xy, endPosNDC.z, screenSizeWithInverse, rayHitPosNDC, iterCount);
    #endif
    float rayLength = 0.0f;
    if (hit)
    {
        float3 endPos = ComputeWorldSpacePosition(rayHitPosNDC.xy, rayHitPosNDC.z, _CameraInverseViewProjections[unity_eyeIndex]);
        rayLength = length(endPos - positionWS);
    }
    return rayLength;
}

#endif // PEMA_SSR_INCLUDED
