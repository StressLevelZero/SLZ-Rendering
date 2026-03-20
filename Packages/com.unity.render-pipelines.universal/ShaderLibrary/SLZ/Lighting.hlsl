#ifndef SLZ_LIGHTING_INCLUDED
#define SLZ_LIGHTING_INCLUDED

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/BRDF.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/GlobalIllumination.hlsl"

namespace SLZ
{

//----------------------------------------------------------------------------
// STRUCTS -------------------------------------------------------------------
//----------------------------------------------------------------------------

    struct LightMeshData
    {
        float3 position;        // worldspace position of the fragment
        half3  normal;          // worldspace normal
        half3  meshNormal;      // raw mesh normal unmodified by normal maps, used for horizon occlusion and SSR
        half3  viewDir;         // normalized camera to fragment vector
        half   NoV;             // normal dot viewDir
        float2 screenUV;        // fragment normalized 0-1 screen uv coordinates
        float2 lightmapUV;      // lightmap uvs (if lightmapped)
        float2 dynLightmapUV;   // dynamic lightmap uvs (if dynamic lightmapped)
        float4 shadowCoord;     // 
        half4  shadowMask;
        half3  vertexLighting;  // total lighting calculated in the vertex, which can contain vertex lights and/or the second order spherical harmonics
    };


    struct LightPhysData
    {
        half3 albedo;
        half  perceptualRoughness;
        half  roughness;
        half3 reflectance;
        half3 emission;
        half  occlusion;
        half  alpha;
        min16int  surfaceType;  
    };

    struct LightMeshDataAniso : LightMeshData
    {
        half3 tangent;
        half3 bitangent;
    };

    struct LightPhysDataAniso : LightPhysData
    {
        half anisoAspect;
        half roughnessT;
        half roughnessB;
        half visLambdaView; //factor used in the anisotropic visibility function that depends on the view, normal, tangent, and bitangent
    };

//----------------------------------------------------------------------------
// FUNCTIONS -----------------------------------------------------------------
//----------------------------------------------------------------------------

        
    /** Isotropic reflection vector from the view vector and normal
     *
     * @param viewDir  normalized worldspace camera to fragment vector
     * @param normal   normalized fragment normal
     * @returns reflection of the view vector about the normal
     */
    half3 ReflectionDirIso(const half3 viewDir, const half3 normal)
    {
        return reflect(-viewDir, normal);
    }

    /** Reflection vector for image-based lighting that gives a psuedo-anisotropic effect by bending the normal
     *  See https://google.github.io/filament/main/filament.html#anisotropy
     *
     * @param viewDir  normalized worldspace camera to fragment vector
     * @param normal   normalized fragment normal
     * @returns bent reflection vector
     */
    half3 ReflectionDirAniso(
        const half3 viewDir, const half3 normal, const half3 tangent, const half3 bitangent, const half anisoAspect)
    {
        half3 anisoDir = anisoAspect > 0 ? bitangent : tangent;
        half3 anisoTangent = cross(anisoDir, viewDir);
        half3 anisoNormal = cross(anisoTangent, anisoDir);
        half3 bentNormal = normalize(lerp(normal, anisoNormal, abs(anisoAspect)));
        
        return reflect(-viewDir, bentNormal);
    }

    /**
     * Specular horizon occlusion factor copied from Filament, which fades out the specular
     * as the reflected ray dips below the surface. This is possible because the normal from
     * normal maps and even smooth interpolated mesh normals aren't geometrically sane.
     * The camera can be below the plane defined by the pixel's normal but still be above 
     * the triangle's true normal plane, meaning that we're still rendering the geometry from
     * the front but the shader thinks were looking at the back. This leads to a reflection
     * vector pointing into the surface.
     *
     * @param   normal          Worldspace normal vector
     * @param   reflectionDir   Reflection vector
     */
    half SpecularHorizonOcclusion(half3 normal, half3 reflectionDir)
    {
        half horizonOcclusion = min(half(1.0h) + dot(reflectionDir, normal), half(1.0h));
        return horizonOcclusion * horizonOcclusion;
    }



    
    half3 ReflectionDir(const LightMeshData mData, const LightPhysDataAniso pData)
    {
        #if defined(SLZ_ANISOTROPIC_SPECULAR)
            return ReflectionDirAniso(mData.viewDir, mData.normal, mData.tangent, mData.bitangent, pData.anisoAspect);
        #else
            return ReflectionDirIso(mData.viewDir, mData.normal);
        #endif
    }


    
//----------------------------------------------------------------------------
// FUNCTION POINTERS ---------------------------------------------------------
//----------------------------------------------------------------------------

    class SpecularModelKSK
    {
        // pre-HLSL 2021, the compiler auto casts structs of the same layout. This makes it impossible to do overloads on struct parameters
        // unless the structs' layouts are different. Solution is to add dummy fields with incompatible types to disambiguate.
        #if !defined(HLSL_2021)
        bool disambiguationKSK;
        #endif
        static half3 CalculateReflectionVector(LightMeshData md, LightPhysData ps)
        {
            return ReflectionDirIso(md.viewDir, md.normal);
        }
        
        static half3 CalculatePunctualSpecular(LightMeshData md, LightPhysData ps, half3 lightDir)
        {
            LagrangeGGXParams ggxParams = LagrangeGGXParams(md.normal, md.viewDir, lightDir);
            return ggxParams.NoL * SpecBrdfFp16KSK(ggxParams, ps.roughness, ps.reflectance);
        }
    };

    class SpecularModelGGX
    {
        #if !defined(HLSL_2021) 
        min16int disambiguationGGX;
        #endif
        static half3 CalculateReflectionVector(LightMeshData md, LightPhysData ps)
        {
            return ReflectionDirIso(md.viewDir, md.normal);
        }
        static half3 CalculatePunctualSpecular(LightMeshData md, LightPhysData ps, half3 lightDir)
        {
            LagrangeGGXParams ggxParams = LagrangeGGXParams(md.normal, md.viewDir, lightDir);
            return ggxParams.NoL * SpecBrdfFp16(ggxParams, md.NoV, ps.roughness, ps.reflectance);
        }
    };

    
    class SpecularModelAniso
    {
        #if !defined(HLSL_2021)
        min16float disambiguationAniso;
        #endif
        static half3 CalculateReflectionVector(LightMeshDataAniso md, LightPhysDataAniso ps)
        {
            return ReflectionDirAniso(md.viewDir, md.normal, md.tangent, md.bitangent, ps.anisoAspect);
        }
        
        static half3 CalculatePunctualSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, half3 lightDir)
        {
            AnisoGGXParams ggxParams = AnisoGGXParams(md.normal, md.tangent, md.bitangent, md.viewDir, lightDir);

            return ggxParams.NoL * SpecBrdfAnisoFp16(ggxParams, md.NoV, ps.roughnessT, ps.roughnessB, ps.anisoAspect, ps.visLambdaView, ps.reflectance);
        }
    };


#ifdef TEMPLATES_SUPPORTED // if HLSL 2021 is available, this include will use real templates
    
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
    
#else // Fake templates by manually defining the type names and including the function multiple times, once for each type we intend to support
    
    // fp16 GGX-KSK Isotropic specular
    #define MESH_DATA LightMeshData
    #define PHYS_DATA LightPhysData
    #define SPECULAR_MODEL SpecularModelKSK
    
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
    
    #undef MESH_DATA
    #undef PHYS_DATA
    #undef SPECULAR_MODEL


    // fp16 GGX isotropic specular using lagrange identity for the visibility function
    #define MESH_DATA LightMeshData
    #define PHYS_DATA LightPhysData
    #define SPECULAR_MODEL SpecularModelGGX
        
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
        
    #undef MESH_DATA
    #undef PHYS_DATA
    #undef SPECULAR_MODEL

    // fp16 GGX Anisotropic specular
    #define MESH_DATA LightMeshDataAniso
    #define PHYS_DATA LightPhysDataAniso
    #define SPECULAR_MODEL SpecularModelAniso
            
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
            
    #undef MESH_DATA
    #undef PHYS_DATA
    #undef SPECULAR_MODEL
#endif
}

#endif
