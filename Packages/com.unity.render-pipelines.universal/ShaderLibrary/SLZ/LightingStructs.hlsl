namespace SLZ
{
    struct LightingMeshData
    {
        float3 position;
        half3 normal;
        half3 viewDir;
        half NoV;
        float2 screenUV;
        float2 lightmapUV;
        float2 dynLightmapUV;
        float4 shadowCoord;
        half4 shadowMask;
        half3 vertexLighting;
#if defined(_SLZ_ANISO_SPECULAR)
        half3 bitangent;
        half3 tangent;
        half visLambdaView; //factor used in the anisotropic visibility function that depends only on the view, normal, tangent, and bitangent
#endif
    };

    struct LightingPhysData
    {
        float3 position;
        half3 normal;
        half3 viewDir;
        half NoV;
        float2 screenUV;
        float2 lightmapUV;
        float2 dynLightmapUV;
        float4 shadowCoord;
        half4 shadowMask;
        half3 vertexLighting;
#if defined(_SLZ_ANISO_SPECULAR)
        half3 bitangent;
        half3 tangent;
        half visLambdaView; //factor used in the anisotropic visibility function that depends only on the view, normal, tangent, and bitangent
#endif
    };
}