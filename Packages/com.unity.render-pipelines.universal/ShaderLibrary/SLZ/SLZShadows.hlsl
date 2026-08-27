#ifndef SLZ_SHADOWS_INCLUDED
#define SLZ_SHADOWS_INCLUDED

// Ignacio Castano http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
float2 GetShadowOffsets( float3 N, float3 L )
{
    float cos_alpha = saturate( dot( N, L ) );
    float offset_scale_N = sqrt( 1 - ( cos_alpha * cos_alpha ) ); // sin( acos( L?N ) )
    float offset_scale_L = offset_scale_N / cos_alpha; // tan( acos( L?N ) )
    return float2( offset_scale_N, min( 2.0, offset_scale_L ) );
}

float4 ApplySLZShadowBias(float3 positionWS, float3 normalWS, float3 lightDirection)
{
    float2 vShadowOffsets = GetShadowOffsets(normalWS, lightDirection);
    //positionWS.xyz -= vShadowOffsets.x * normalWS.xyz * .003;
    positionWS.xyz -= vShadowOffsets.y * lightDirection.xyz * 0.01; //_ShadowBias.x    
    return TransformWorldToHClip(positionWS.xyz);
}

#endif