#if !defined(FIXED_ADDRESS_LIGHTS_INCLUDED)
#define FIXED_ADDRESS_LIGHTS_INCLUDED

// On Adreno gpu's (Quest), accessing an array inside of a constant buffer with
// an index that is not a compile-time constant is extremely costly. The driver seems to
// precompute exactly what values are accessed so it can copy those to tile memory.
// However, using a non-costexpr address prevents it from doing so, and instead
// it loads the cbuffer from GMEM like a structured buffer.
//
// Unity's forward rendering uses an indirection system where all onscreen lights are
// provided in a structure of arrays, and the indices of the lights that are assigned
// to the current draw are specified in a float4[2]. Even though these indices are
// cbuffer constants, because they are not known at compile-time this tanks 
// performance on the Quest.

#define MAX_FIXED_ADDRESS_LIGHTS 4

#if defined(SLZ_FIXED_ADDRESS_LIGHTS)

cbuffer FixedAddressLights
{
    float4 _FixedLightPositions         [MAX_FIXED_ADDRESS_LIGHTS]; // 0
    float4 _FixedLightColors            [MAX_FIXED_ADDRESS_LIGHTS]; // 64
    float4 _FixedLightAttenuations      [MAX_FIXED_ADDRESS_LIGHTS]; // 128
    float4 _FixedLightSpotDirs          [MAX_FIXED_ADDRESS_LIGHTS]; // 192
    uint4 _FixedLightIndices; // 256
    int  _FixedLightCount; // 272
};

#endif

Light GetFixedAddressAdditionalLight(int index, float3 positionWS)
{
#if defined(SLZ_FIXED_ADDRESS_LIGHTS)
    float4 lightPositionWS = _FixedLightPositions[index];
    LIGHT_VEC color = _FixedLightColors[index].LIGHT_SWIZZLE;
    half4 distanceAndSpotAttenuation = _FixedLightAttenuations[index];
    half4 spotDirection = _FixedLightSpotDirs[index];

    float3 lightVector = lightPositionWS.xyz - positionWS * lightPositionWS.w;
    float distanceSqr = max(dot(lightVector, lightVector), HALF_MIN);

    half3 lightDirection = half3(lightVector * rsqrt(distanceSqr));

    float attenuation = DistanceAttenuation(distanceSqr, distanceAndSpotAttenuation.xy) * AngleAttenuation(spotDirection.xyz, lightDirection, distanceAndSpotAttenuation.zw);
    
    Light light;
    light.direction = lightDirection;
    light.distanceAttenuation = attenuation;
    light.shadowAttenuation = 1.0; 
    light.color = color;
    light.layerMask = 0xFFFFFFFFu;

    return light;
#else
    Light light = (Light)0;
    return light;
#endif
}

#endif
