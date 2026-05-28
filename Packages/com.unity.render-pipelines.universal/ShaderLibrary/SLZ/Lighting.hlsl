#ifndef SLZ_LIGHTING_INCLUDED
#define SLZ_LIGHTING_INCLUDED

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/BRDF.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/FGD.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/GlobalIllumination.hlsl"

namespace SLZ
{

//----------------------------------------------------------------------------
// STRUCTS -------------------------------------------------------------------
//----------------------------------------------------------------------------

#if defined(UNITY_COMPILER_DXC)
    enum SurfaceType : min16int
    {
        Opaque      = 0,
        Transparent = 1,
        Fade        = 2
    };
#else
    class SurfaceType
    {
        static const min16int Opaque      = 0;
        static const min16int Transparent = 1;
        static const min16int Fade        = 2;
    };
#endif

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
        half  roughness;
        half3 specularF0; // Specular reflectance at normal incidence
        half3 emission;
        half  occlusion;
        half  alpha;

        // Surface type 
        min16int  surfaceType;

        // Reduce register use by not storing the perceptual roughness 
        half PerceptualRoughness()
        {
            return sqrt(roughness);
        }

        // Switch to metallic only version that uses a scalar times the albedo rather than 
        // using two more registers to store the specular reflectance vector? This has
        // to be used at every stage, might be better to incur the cost of doing several
        // multiplies if it gets us under a register usage threshold
        half3 SpecularF0()
        {
            return specularF0;
        }

        half3 SpecularF90()
        {
            return half3(1,1,1);
        }
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


    /**
     * Image based specular using the multiscatter fdg/dfg lookup table. This requires reflection probes to have custom mips 
     * containing convolutions calculated according to the multiscatter LD formula. See IntegrateGGXAndDisneyDiffuseFGD in
     * Packages/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl. 
     * The default built-in pipeline gaussian blurred mips will not give a correct result!!!
     *
     * This is using the SRP Core/HDRP use the multiscatter dfg formula also used by the Google Filament renderer 
     * (https://google.github.io/filament/Filament.md.html#listing_multiscatteriblevaluation) 
     * I have no clue what the actual source for this formula is, unity erroneously references the non-multiscatter formula used by 
     * "Moving Frostbite to PBR" and the Filament doc doesn't properly cite its sources (maybe Filament is the primary source?). 
     */
    half3 CalculateImageBasedSpecularMultiscatterFDG(half3 reflectionDir, float3 position, half roughness, float2 screenUV, half NoV, half3 specularF0, half2 fdg)
    {
        half3 rawReflection = GlossyEnvironmentReflection(reflectionDir, position, sqrt(max(HALF_MIN, roughness)), 1.0f, screenUV);
        return lerp(fdg.xxx, fdg.yyy, specularF0) * rawReflection;
    }

    /** 
     * Image based specular using the built-in pipeline's non-physically based surface reduction fudge factor + pow4 fresnel
     */
    half3 CalculateImageBasedSpecularNonPhys(half3 reflectionDir, float3 position, half roughness, float2 screenUV, half NoV, half3 specularF0)
    {
        half3 rawReflection = GlossyEnvironmentReflection(reflectionDir, position, sqrt(max(HALF_MIN, roughness)), 1.0f, screenUV);
        half unitySurfaceReduction = half(1.0) / (roughness * roughness + half(1.0));
        half fresnel = half(1.0) - saturate(NoV);
        fresnel *= fresnel;
        fresnel *= fresnel;
        return rawReflection * unitySurfaceReduction * lerp(specularF0, half3(1,1,1), fresnel);
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
            return ggxParams.NoL * SpecBrdfFp16KSK(ggxParams, ps.roughness, ps.SpecularF0());
        }

        static half3 CalculateIBLSpecular(LightMeshData md, LightPhysData ps, half3 reflectionDir, half2 fgd)
        {
            return CalculateImageBasedSpecularMultiscatterFDG(reflectionDir, md.position, ps.roughness, md.screenUV, md.NoV, ps.SpecularF0(), fgd);
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
            return ggxParams.NoL * SpecBrdfFp16(ggxParams, md.NoV, ps.roughness, ps.SpecularF0());
        }
        static half3 CalculateIBLSpecular(LightMeshData md, LightPhysData ps, half3 reflectionDir, half2 fgd)
        {
            return CalculateImageBasedSpecularMultiscatterFDG(reflectionDir, md.position, ps.roughness, md.screenUV, md.NoV, ps.SpecularF0(), fgd);
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

            return ggxParams.NoL * SpecBrdfAnisoFp16(ggxParams, md.NoV, ps.roughnessT, ps.roughnessB, ps.anisoAspect, ps.visLambdaView, ps.SpecularF0());
        }

        static half3 CalculateIBLSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, half3 reflectionDir, half2 fgd)
        {
            return CalculateImageBasedSpecularMultiscatterFDG(reflectionDir, md.position, ps.roughness, md.screenUV, md.NoV, ps.SpecularF0(), fgd);
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
