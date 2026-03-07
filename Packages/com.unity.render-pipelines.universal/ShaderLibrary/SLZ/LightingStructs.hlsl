namespace SLZ
{
    struct LightMeshData
    {
        float3 position;
        half3  normal;
        half3  viewDir;
        half   NoV;
        float2 screenUV;
        float2 lightmapUV;
        float2 dynLightmapUV;
        float4 shadowCoord;
        half4  shadowMask;
        half3  vertexLighting;
    };

    struct LightMeshDataAniso : LightMeshData
    {
        half3 bitangent;
        half3 tangent;
        half  visLambdaView; //factor used in the anisotropic visibility function that depends on the view, normal, tangent, and bitangent
    };

    struct LightPhysData
    {
        half3 albedo;
        half  perceptualRoughness;
        half  roughness;
        half3 specular;
        half3 emission;
        half  occlusion;
        half  alpha;
    };

    struct LightPhysDataAniso : LightPhysData
    {
        half anisoAspect;
        half roughnessT;
        half roughnessB;
    };
}
