#ifndef SLZ_LIGHTING_INCLUDED
#define SLZ_LIGHTING_INCLUDED

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Constants.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/BRDF.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/FGD.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/GlobalIllumination.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/RealtimeLights.hlsl"

namespace SLZ
{

//----------------------------------------------------------------------------
// STRUCTS -------------------------------------------------------------------
//----------------------------------------------------------------------------

#if defined(UNITY_COMPILER_DXC)
    enum SurfaceType : min16uint
    {
        Opaque      = 0,
        // Physical transmission of light through a solid, specular reflections are unaffected by the transparency
        Transparent = 1,
        // Transparency that affects all components of the final color, straight alpha blending with the background
        Fade        = 2
    };
#else
    class SurfaceType
    {
        static const min16uint Opaque      = 0;
        static const min16uint Transparent = 1;
        static const min16uint Fade        = 2;
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
        half3  vertexLighting;  // total lighting calculated in the vertex, which can contain vertex lights and/or the second order spherical harmonics
    };


    struct LightPhysData
    {
        // Reduce register usage by packing long-lived color values to 8-bit fixed point
        #ifdef PACK_COLOR_UNORM4X8
            uint packedAlbedoAlpha;
            uint packedSpecularF0Roughness;
        #else
            half3 m_albedo;
            half  m_alpha;
            half3 m_specularF0;
            half  m_roughness;
        #endif

        half3 emission;
        half  occlusion;


        // Surface type 
        min16uint  surfaceType;

        // Try to reduce register use by not storing the perceptual roughness 
        half PerceptualRoughness()
        {
            return sqrt(Roughness());
        }

        half4 AlbedoAlpha()
        {
            #ifdef PACK_COLOR_UNORM4X8
                half4 unpacked = VkSPIRV::UnpackUNorm4x8(packedAlbedoAlpha);
                return half4(unpacked.rgb * unpacked.rgb, unpacked.a);
            #else
                return half4(m_albedo, m_alpha);
            #endif
        }

        void SetAlbedoAlpha(half3 albedo, half alpha)
        {
            #ifdef PACK_COLOR_UNORM4X8
                packedAlbedoAlpha = VkSPIRV::PackUNorm4x8(half4(sqrt(albedo), alpha));
            #else
                m_albedo = albedo;
                m_alpha = alpha;
            #endif
        }
        
        half3 SpecularF0()
        {
            #ifdef PACK_COLOR_UNORM4X8
                half4 unpacked = VkSPIRV::UnpackUNorm4x8(packedSpecularF0Roughness);
                return unpacked.xyz * unpacked.xyz;
            #else
                return m_specularF0;
            #endif
        }

        half Roughness()
        {
            #ifdef PACK_COLOR_UNORM4X8
                return VkSPIRV::UnpackUNorm4x8(packedSpecularF0Roughness).w;
            #else
                return m_roughness;
            #endif
        }

        void SetSpecularF0RoughnessFromMetallic(half metallic, half roughness)
        {
            #ifdef PACK_COLOR_UNORM4X8
                half4 albedoAlpha = AlbedoAlpha();
                half3 specularF0 = lerp(half3(kDielectricSpec.xyz), albedoAlpha.rgb, metallic);
                packedSpecularF0Roughness = VkSPIRV::PackUNorm4x8(half4(sqrt(specularF0), roughness));
                SetAlbedoAlpha(albedoAlpha.rgb * (half(1.0) - metallic), albedoAlpha.a);
            #else
                m_specularF0 = lerp(half3(kDielectricSpec.xyz), m_albedo, metallic);
                m_albedo *= half(1.0) - metallic;
                m_roughness = roughness;
            #endif
        }

        void SetSpecularF0Roughness(half3 specularF0, half roughness)
        {
            #ifdef PACK_COLOR_UNORM4X8
                packedSpecularF0Roughness = VkSPIRV::PackUNorm4x8(half4(sqrt(specularF0), roughness));
            #else
                m_specularF0 = specularF0;
                m_roughness = roughness;
            #endif
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

    struct ShCoefficients
    {
        half4 L1L0r;
        half4 L1L0g;
        half4 L1L0b;

        half4 L2Br;
        half4 L2Bg;
        half4 L2Bb;
        half3 L2C;

        half4 probeOcclusion;
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
     * Specular horizon occlusion factor modified from Filament, which fades out the specular
     * as the reflected ray dips below the normal plane. This additionally fades out as the
     * reflected vector dips below the unmodified geometry normals, as normal maps commonly
     * cause the reflection direction to point into the physical geometry, even if the camera is
     * above the plane defined by the normal map
     *
     * @param   normal          Worldspace normal vector
     * @param   reflectionDir   Reflection vector
     * @returns Multiplier for the specular which smoothly reduces it to 0 as the reflection vector dips into the surface
     */
    half SpecularHorizonOcclusion(half3 normal, half3 meshNormal, half3 reflectionDir)
    {
        half NoR = //min(dot(reflectionDir, normal), dot(reflectionDir, meshNormal));
                    dot(reflectionDir, normal);
        half horizonOcclusion = min(half(1.0h) + NoR, half(1.0h));
        return horizonOcclusion * horizonOcclusion;
    }

    /** 
     * Falloff function for a fake specular highlight added based on assuming global illumination 
     * is partially coming from a punctual light in a given direction. 
     *
     * Since the angular falloff NoL is baked into the GI light intensity, if we 
     * multiply by NoL like in the standard punctual specular formula the angular 
     * falloff will be too strong. However, we also need to remove all specular 
     * response from normals facing away from the light direction, and GI rarely
     * reduces to 0 in any direction. This function is a sharpened NoL similar to
     * sqrt(NoL), which will not highten the angular response as much while smoothly
     * reducing the specular response to 0 as NoL goes to 0.
     */
    half GiFakedSpecularFalloff(half NoL)
    {
	    half NoLMul = half(1.0) - saturate(NoL);
	    NoLMul = -NoLMul * NoLMul + half(1.0);
        return NoLMul;
    }


    /**
     * Image based lighting specular from reflection probes using the multiscatter fgd/dfg lookup table. 
     * This requires reflection probes to have custom mips containing convolutions calculated according 
     * to the multiscatter LD formula. See IntegrateGGXAndDisneyDiffuseFGD and IntegrateLD in
     * Packages/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl. 
     * The default built-in pipeline mips will not give a correct result!!!
     *
     * This is using the SRP Core/HDRP use the multiscatter dfg formula also used by the Google Filament renderer 
     * (https://google.github.io/filament/Filament.md.html#listing_multiscatteriblevaluation) 
     * I have no clue what the actual source for this formula is, unity erroneously references the non-multiscatter formula used by 
     * "Moving Frostbite to PBR" and the Filament doc doesn't properly cite its sources (maybe Filament is the primary source?). 
     * 
     * @param reflectionDir Normalized reflection vector
     * @param position      Worldspace position of the fragment
     * @param roughness     Linear roughness
     * @param screenUV      0-1 screen coordinates of the fragment
     * @param NoV           normal dot viewDir
     * @param specularF0    specular reflectance at normal incidence
     * @param fgd           value from the FGD lookup table for the fragment's NoV and roughness
     *
     * @returns Image based specular from the reflection probe cubemap sources
     */
    half3 ProbeIblSpecularMultiscatterFGD(half3 reflectionDir, float3 position, half roughness, float2 screenUV, half NoV, half3 specularF0, half2 fgd)
    {
        half3 rawReflection = GlossyEnvironmentReflection(reflectionDir, position, sqrt(max(HALF_MIN, roughness)), 1.0f, screenUV);
        return lerp(fgd.xxx, fgd.yyy, specularF0) * rawReflection;
    }

    /**
     * Multiscatter energy loss compensation for the punctual specular BRDF using the precomputed terms from the FGD lookup table.
     * See https://google.github.io/filament/Filament.md.html#materialsystem/improvingthebrdfs/energylossinspecularreflectance
     */
    half3 PunctualSpecularMultiscatterComp(half3 specular, half3 specularF0, half2 fgd)
    {
        return specular * (half(1.0) + specularF0 * (rcp(fgd.y) - half(1.0)));
    }


    /** 
     * Image based lighting specular from reflection probes using unity's default non-physical BDRF formula
     * 
     * @param reflectionDir Normalized reflection vector
     * @param position      Worldspace position of the fragment
     * @param roughness     Linear roughness
     * @param screenUV      0-1 screen coordinates of the fragment
     * @param NoV           normal dot viewDir
     * @param specularF0    specular reflectance at normal incidence
     * @param fgd           value from the FGD lookup table for the fragment's NoV and roughness
     *
     * @returns Image based specular from the reflection probe cubemap sources
     */
    half3 ProbeIblSpecularNonPhys(half3 reflectionDir, float3 position, half roughness, float2 screenUV, half NoV, half3 specularF0)
    {
        half3 rawReflection = GlossyEnvironmentReflection(reflectionDir, position, sqrt(max(HALF_MIN, roughness)), 1.0f, screenUV);
        half unitySurfaceReduction = half(1.0) / (roughness * roughness + half(1.0));
        half fresnel = half(1.0) - saturate(NoV);
        fresnel *= fresnel;
        fresnel *= fresnel;
        return rawReflection * unitySurfaceReduction * lerp(specularF0, half3(1,1,1), fresnel);
    }

    /**
     * Get the diffuse GI spherical harmonic coefficients and the occlusion probe (if shadowmasking is on)
     * 
     * @param positionWS  Worldspace position
     * @param normalWS    Worldspace normal
     * @param viewDir     Worldspace camera to fragment vector (normalized)
     * @param renderingLayer  Unity's rendering layer bitmask
     *
     * @returns struct containing the L0, L1, and L2 spherical harmonic coefficients and the occlusion probe
     */
    ShCoefficients SphericalHarmonicCoefficients(float3 positionWS, half3 normalWS, half3 viewDir, uint renderingLayer)
    {
        ShCoefficients result = (ShCoefficients)0;
        #if defined(PROBE_VOLUMES_L1)
            APVSample apvSample = SampleAPV(positionWS, normalWS, renderingLayer, viewDir);
            apvSample.Decode();
            bool success = apvSample.status != APV_SAMPLE_STATUS_INVALID;

            result.L1L0r = select(success, half4(apvSample.L1_R, apvSample.L0.r), unity_SHAr);
            result.L1L0g = select(success, half4(apvSample.L1_G, apvSample.L0.g), unity_SHAg);
            result.L1L0b = select(success, half4(apvSample.L1_B, apvSample.L0.b), unity_SHAb);

            result.probeOcclusion = apvSample.probeOcclusion;
            
        #else

            result.L1L0r = unity_SHAr;
            result.L1L0g = unity_SHAg;
            result.L1L0b = unity_SHAb;
            result.L2Br = unity_SHBr;
            result.L2Bg = unity_SHBg;
            result.L2Bb = unity_SHBb;
            result.L2C = unity_SHC;
            #if defined(SHADOWS_SHADOWMASK)
                result.probeOcclusion = unity_ProbesOcclusion;
            #else
                result.probeOcclusion = (half4)0;
            #endif
        #endif
        return result;
    }

    /** Copy of GetMainLight from RealtimeLights.hlsl that takes worldspace position and shadowCoord instead of URP lit's entire inputData struct
     *  
     * @param shadowCoord   Shadow coordinates
     * @param shadowMask    Shadow mask value from probes if LIGHTMAP_SHADOW_MIXING is enabled
     * @param aoFactor      Struct containing screenspace ambient occlusion if present and the ambient occlusion texture value.
     * 
     * @returns             Light struct with the information for the main directional light
     */
    Light GetMainLight(float3 positionWS, float4 shadowCoord, half4 shadowMask, AmbientOcclusionFactor aoFactor)
    {
        Light light = GetMainLight(shadowCoord, positionWS, shadowMask);

        #if defined(_SCREEN_SPACE_OCCLUSION) && !defined(_SURFACE_TYPE_TRANSPARENT)
        if (IsLightingFeatureEnabled(DEBUGLIGHTINGFEATUREFLAGS_AMBIENT_OCCLUSION))
        {
            light.color *= aoFactor.directAmbientOcclusion;
        }
        #endif

        return light;
    }

    /**
     * Get a fake point source direction from diffuse GI spherical harmonics 
     *
     * A crude attempt to get a specular highlight from light probes. The L1 ceofficient is the same shape
     * as the lambert diffuse lobe from an infinitely far light source, so it stands to reason that the L1
     * will often represent the light contribution from a single source. Averaging the L1 r,g,b vectors will 
     * give us a light direction we can use to create a specular highlight. This works well in many situations,
     * but there are also plenty of others where it falls apart. If there isn't a strong unidirectional component 
     * to the light, the L1 vectors will point in seemingly random and rapidly spatially varying directions.
     * Still better than nothing. Baking direct specular into reflection probes by giving light sources physical
     * emissive meshes the probe can see is a superior solution in many situations. However it needs box projection
     * and probe blending to look good, which we aren't doing on the quest.
     *
     * @param sh Spherical harmonic coefficients
     *
     * @returns Average direction of the spherical harmonic L1 vectors, normalized 
     */
    half3 ShFakeSpecularDirection(ShCoefficients sh)
    {
        float3 direction = (sh.L1L0r.xyz + sh.L1L0g.xyz + sh.L1L0b.xyz);
        float lengthSq = max(float(dot(direction, direction)), SAFE_FLT_RSQRT_MIN);
        float invLength = rsqrt(lengthSq);
        direction = direction * invLength;
        return direction;
    }

    /**
     * Samples a lightmap texture. Handles lightmap arrays when using DOTS and bicubic filtering. Do not use for Enlighten dynamic lightmaps as those are
     * never 2DArrays, even when using DOTS
     *
     * @param lightmap          A Texture2D or Texture2DArray if using DOTS representing the lightmap (use the LIGHTMAP_NAME, LIGHTMAP_INDIRECTION_NAME, or SHADOWMASK_NAME macros as those resolve to the correct type for the current setup)
     * @param samplerState      The sampler state to use (use LIGHTMAP_SAMPLER_NAME or SHADOWMASK_SAMPLER_NAME)
     * @param staticLightmapUV  Lightmap UVs post lightmap scale/offset transformation
     * @param bicubic           Use bicubic filtering
     *
     * @returns The raw value of the lightmap texture for the given UV
     */
    half4 SampleLightmapTexture(TEXTURE2D_LIGHTMAP_PARAM(lightmap, samplerState), float2 staticLightmapUV, bool bicubic)
    {
        half4 output;
        if (bicubic)
        {
            // LIGHTMAP_SAMPLE_EXTRA_ARGS defined in GlobalIllumination.hlsl to be "staticLightmapUV, unity_LightmapIndex.x" when using dots instancing and non-legacy lightmaps
            output = SampleLightmapBicubic(TEXTURE2D_LIGHTMAP_ARGS(lightmap, samplerState), LIGHTMAP_SAMPLE_EXTRA_ARGS);
        }
        else
        {
            output = SAMPLE_TEXTURE2D_LIGHTMAP(lightmap, samplerState, LIGHTMAP_SAMPLE_EXTRA_ARGS);
        }

        // Normally, unity "decodes" lightmaps because it assumes support for ancient graphics API's with no native HDR image format (gles2) where it is stored as shared exponent floats encoded in a fixed point texture, and must be decoded manually
        // This is no longer relevant for any hardware made in the past decade. Don't waste instructions on this!
        // half4 decodeInstructions = half4(LIGHTMAP_HDR_MULTIPLIER, LIGHTMAP_HDR_EXPONENT, 0.0, 0.0);
        // output.rgb = DecodeLightmap(output, decodeInstructions);

        return output;
    }

    /**
     * Applies the unity default "half lambert" directional lightmap to the base lightmap's color.
     * 
     * @param normalWS      World-space normal
     * @param baseLightmap  The main lightmap's lambert irradiance value unaffected by the albedo
     * @param dirLightmap   The directional lightmap's raw value
     *
     * @returns  The lightmap's irradiance modified by the "half-lambert" directionality
     */
    half3 ApplyDirLightmapHalfLambert(const half3 normalWS, const half3 baseLightmap, const half4 dirLightmap)
    {
        half halfLambert = dot(normalWS, dirLightmap.xyz - half(0.5)) + half(0.5);
        return baseLightmap * halfLambert / max(half(1e-4), dirLightmap.w);
    }

    /**
     * Decodes spherical harmonic diffuse irradiance where the L0 coefficient for each color channel
     * is stored in the main lightmap, and the directional lightmap contains monochromatic 
     * L1 coefficients scaled by the L0 coefficients to fit in the 0-1 range. These maps can be created with Bakery.
     * This function only does simple L1 SH and does not do windowing or hallucinated ZH3 harmonics
     * 
     * @param normalWS      World-space normal
     * @param baseLightmap  The main lightmap containing the L0 coefficients
     * @param dirLightmap   The directional lightmap containing the L1 coefficients
     *
     * @returns  The lambert diffuse L1 spherical harmonic irradiance for the given normal direction
     */
    half3 DecodeDirLmMonoShSimple(const half3 normalWS, const half3 baseLightmap, const half3 dirLightmap)
    {
        half3 L1Mono = 4.0 * dirLightmap - 2.0;
        return baseLightmap * (half(1.0) + dot(normalWS, L1Mono));
    }


    /**
     * Applies the directional lightmap to the base lightmap. 
     * 
     * @param normalWS      Fragment normal
     * @param baseLightmap  Main lightmap value
     * @param dirLightmap   Directional lightmap value
     *
     * @returns Diffuse irradiance stored in the lightmaps for the given normal direction
     */
    half3 ApplyDirLightmap(const half3 normalWS, const half3 baseLightmap, const half4 dirLightmap)
    {
        #if defined(DIRLIGHTMAP_MONOSH)
            return DecodeDirLmMonoShSimple(normalWS, baseLightmap, dirLightmap);
        #else
            return ApplyDirLightmapHalfLambert(normalWS, baseLightmap, dirLightmap);
        #endif
    }

    /**
     * Calculate the direction for a fake punctual specular highlight and a strength
     * multiplier from a directional lightmap. The strength multiplier attempts to fade 
     * out the highlight when the lighting is not strongly unidirectional. This is determined
     * by the length of the half lambert direction vector or the L1 monoSH coefficient vector
     * stored in the directional lightmap; as the length approaches the value calculated for 
     * a white furnace, the strength goes to 0. 
     *
     * @param dirLightmap   Raw value of the directional lightmap
     *
     * @returns half4 containing a normalized direction vector in xyz and the specular strength in w
     */
    half4 LightmapFakeSpecStrengthDir(const half4 dirLightmap)
    {
        half3 lmDirection = half(2.0) * dirLightmap.xyz - half(1.0);

        #if defined(DIRLIGHTMAP_MONOSH)
            half whiteFurnaceLen = half(0.54);
        #else
            half whiteFurnaceLen = half(0.66);
        #endif

        float lmDirSq = dot(float3(lmDirection), float3(lmDirection) );
        float lmDirInvLen = rsqrt(max(lmDirSq, SAFE_FLT_RSQRT_MIN));
        lmDirection *= lmDirInvLen;
        half strength = saturate((rcp(lmDirInvLen) - whiteFurnaceLen) / (half(1.0) - whiteFurnaceLen));

        return half4(lmDirection, strength);
    }



//----------------------------------------------------------------------------
// FUNCTION POINTERS ---------------------------------------------------------
//----------------------------------------------------------------------------

    class DiffuseModelLambert
    {
        // pre-HLSL 2021, the compiler auto casts structs of the same layout. This makes it impossible to do overloads on struct parameters
        // unless the structs' layouts are different. Solution is to add dummy fields with incompatible types to disambiguate.
        #if !defined(HLSL_2021)
        uint disambiguationLambert;
        #endif

        static half4 PunctualDiffuse(LightMeshData md, LightPhysData ps, half3 lightDir, half4 lightIntensity)
        {
            return half4(saturate(dot(lightDir, md.normal)) * lightIntensity);
        }

        static half3 ShDiffuse(LightMeshData md, LightPhysData ps, ShCoefficients sh)
        {
            half3 result = half3(
                            dot(md.normal, sh.L1L0r.rgb) + sh.L1L0r.a,
                            dot(md.normal, sh.L1L0g.rgb) + sh.L1L0g.a,
                            dot(md.normal, sh.L1L0b.rgb) + sh.L1L0b.a
                        );
            #if !defined(PROBE_VOLUMES_L1) && !defined(EVALUATE_SH_MIXED)
                result += SHEvalLinearL2(md.normal, sh.L2Br, sh.L2Bg, sh.L2Bb, half4(sh.L2C, 0.0));
            #endif

            return max(0, result);
        }

        static half3 LightmapDiffuse(LightMeshData md, LightPhysData ps, half3 lightmapTexel, half4 dirLmTexel)
        {
            half3 result = (half3)0;
            #if defined(LIGHTMAP_ON) || defined(DIRLIGHTMAP_COMBINED)
            result = lightmapTexel;
            #endif

            #if defined(DIRLIGHTMAP_COMBINED)
            result = ApplyDirLightmap(md.normal, result, dirLmTexel);
            #endif

            return result;
        }
    };

    class SpecularModelKSK
    {
        #if !defined(HLSL_2021)
        bool disambiguationKSK;
        #endif
        static half3 ReflectionVector(LightMeshData md, LightPhysData ps)
        {
            return ReflectionDirIso(md.viewDir, md.normal);
        }
        
        static half3 PunctualSpecular(LightMeshData md, LightPhysData ps, half3 lightDir, half2 fgd)
        {
            LagrangeGGXParams ggxParams = LagrangeGGXParams(md.normal, md.viewDir, lightDir);
            half3 specular = ggxParams.NoL * SpecBrdfFp16KSK(ggxParams, ps.Roughness(), ps.SpecularF0());
            return PunctualSpecularMultiscatterComp(specular, ps.SpecularF0(), fgd);
        }

        static half3 IblSpecular(LightMeshData md, LightPhysData ps, half3 reflectionDir, half2 fgd)
        {
            return ProbeIblSpecularMultiscatterFGD(reflectionDir, md.position, ps.Roughness(), md.screenUV, md.NoV, ps.SpecularF0(), fgd) 
                    * SpecularHorizonOcclusion(md.normal, md.meshNormal, reflectionDir)
                    ;
        }

        static half3 ShFakeSpecular(LightMeshData md, LightPhysData ps, ShCoefficients sh, half3 ShDiffuse, half2 fgd)
        {
            half3 fakeLightDirection = ShFakeSpecularDirection(sh);
            half3 fakeSpecular = SpecularModelKSK::PunctualSpecular(md, ps, fakeLightDirection, fgd);
            half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirection));
            return ShDiffuse * ps.SpecularF0() * fakeSpecular * fakeFalloff * SH_L1_IRR_TO_RAD;
        }

        static half3 LightmapFakeSpecular(LightMeshData md, LightPhysData ps, half3 lightmapIrradiance, half4 dirLightmap, half2 fgd)
        {
            half4 fakeLightDirStrength = LightmapFakeSpecStrengthDir(dirLightmap);
            half3 fakeSpecular = SpecularModelKSK::PunctualSpecular(md, ps, fakeLightDirStrength.xyz, fgd);

            //half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirection));
            half lightmapRadiance = half(TWO_PI) * lightmapIrradiance; // Lambert 1/2pi factor baked into lightmap directly
            return max(half(0), half(TWO_PI) * lightmapIrradiance * ps.SpecularF0() * fakeSpecular * fakeLightDirStrength.w);
        }
    };

    class SpecularModelGGX
    {
        #if !defined(HLSL_2021) 
        min16int disambiguationGGX;
        #endif
        static half3 ReflectionVector(LightMeshData md, LightPhysData ps)
        {
            return ReflectionDirIso(md.viewDir, md.normal);
        }
        static half3 PunctualSpecular(LightMeshData md, LightPhysData ps, half3 lightDir, half2 fgd)
        {
            LagrangeGGXParams ggxParams = LagrangeGGXParams(md.normal, md.viewDir, lightDir);
            half3 specular = ggxParams.NoL * SpecBrdfFp16(ggxParams, md.NoV, ps.Roughness(), ps.SpecularF0());
            return PunctualSpecularMultiscatterComp(specular, ps.SpecularF0(), fgd);
        }

        
        static half3 IblSpecular(LightMeshData md, LightPhysData ps, half3 reflectionDir, half2 fgd)
        {
            return ProbeIblSpecularMultiscatterFGD(reflectionDir, md.position, ps.Roughness(), md.screenUV, md.NoV, ps.SpecularF0(), fgd)
                    * SpecularHorizonOcclusion(md.normal, md.meshNormal, reflectionDir)
                    ;
        }

        static half3 ShFakeSpecular(LightMeshData md, LightPhysData ps, ShCoefficients sh, half3 ShDiffuse, half2 fgd)
        {
            half3 fakeLightDirection = ShFakeSpecularDirection(sh);
            half3 fakeSpecular = SpecularModelGGX::PunctualSpecular(md, ps, fakeLightDirection, fgd);
            half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirection));
            return ShDiffuse * ps.SpecularF0() * fakeSpecular * fakeFalloff;
        }

        static half3 LightmapFakeSpecular(LightMeshData md, LightPhysData ps, half3 lightmapRadiance, half4 dirLightmap, half2 fgd)
        {
            half4 fakeLightDirStrength = LightmapFakeSpecStrengthDir(dirLightmap);
            half3 fakeSpecular = SpecularModelGGX::PunctualSpecular(md, ps, fakeLightDirStrength.xyz, fgd);

            //half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirection));
            half lightmapIrradiance = half(TWO_PI) * lightmapRadiance; // Lambert 1/2pi factor baked into lightmap directly
            return max(half(0), half(TWO_PI) * lightmapRadiance * ps.SpecularF0() * fakeSpecular * fakeLightDirStrength.w);
        }
    };

    
    class SpecularModelAniso
    {
        #if !defined(HLSL_2021)
        min16float disambiguationAniso;
        #endif
        static half3 ReflectionVector(LightMeshDataAniso md, LightPhysDataAniso ps)
        {
            return ReflectionDirAniso(md.viewDir, md.normal, md.tangent, md.bitangent, ps.anisoAspect);
        }
        
        static half3 PunctualSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, half3 lightDir, half2 fgd)
        {
            AnisoGGXParams ggxParams = AnisoGGXParams(md.normal, md.tangent, md.bitangent, md.viewDir, lightDir);

            half3 specular = ggxParams.NoL * SpecBrdfAnisoFp16(ggxParams, md.NoV, ps.roughnessT, ps.roughnessB, ps.anisoAspect, ps.visLambdaView, ps.SpecularF0());
            return  PunctualSpecularMultiscatterComp(specular, ps.SpecularF0(), fgd);
        }

        static half3 IblSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, half3 reflectionDir, half2 fgd)
        {
            return ProbeIblSpecularMultiscatterFGD(reflectionDir, md.position, ps.Roughness(), md.screenUV, md.NoV, ps.SpecularF0(), fgd)
                    * SpecularHorizonOcclusion(md.normal, md.meshNormal, reflectionDir)
                    ;
        }

        static half3 ShFakeSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, ShCoefficients sh, half3 ShDiffuse, half2 fgd)
        {
            half3 fakeLightDirection = ShFakeSpecularDirection(sh);
            half3 fakeSpecular = SpecularModelAniso::PunctualSpecular(md, ps, fakeLightDirection, fgd);
            half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirection));
            return ShDiffuse * ps.SpecularF0() * fakeSpecular * fakeFalloff;
        }

        static half3 LightmapFakeSpecular(LightMeshDataAniso md, LightPhysDataAniso ps, half3 lightmapRadiance, half4 dirLightmap, half2 fgd)
        {
            half4 fakeLightDirStrength = LightmapFakeSpecStrengthDir(dirLightmap);
            half3 fakeSpecular = SpecularModelGGX::PunctualSpecular(md, ps, fakeLightDirStrength.xyz, fgd);

            half fakeFalloff = GiFakedSpecularFalloff(dot(md.normal, fakeLightDirStrength.xyz));
            half lightmapIrradiance = half(TWO_PI) * lightmapRadiance; // Lambert 1/2pi factor baked into lightmap directly
            return max(half(0), lightmapIrradiance * ps.SpecularF0() * fakeSpecular * fakeLightDirStrength.w * fakeFalloff);
        }
    };

//----------------------------------------------------------------------------
// LIGHTING FUNCTION TEMPLATES -----------------------------------------------
//----------------------------------------------------------------------------

#ifdef TEMPLATES_SUPPORTED // if HLSL 2021 is available, this include will use real templates
    
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
    
#else // Fake templates by manually defining the type names and including the function multiple times, once for each type we intend to support
    #define DIFFUSE_MODEL DiffuseModelLambert
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

    #undef DIFFUSE_MODEL
#endif
}

#endif
