#if !defined(LIGHTING_MODEL_BRDF_LUT_INCLUDED)
#define LIGHTING_MODEL_BRDF_LUT_INCLUDED

namespace SLZ
{
    class DiffuseModelSkinBrdfLut
    {
        Texture2D BRDFLut;
        half3 SubSurfScatterIntensity;

        static half SHEvalLinearL2Channel(half3 N, half4 shB, half shCChan)
        {
            half4 vB = N.xyzz * N.yzzx;
            half  x2 = dot(shB, vB);
            half  vC = N.x * N.x - N.y * N.y;
            return x2 + shCChan * vC;
        }

        /**
         * Samples a 2D BRDF lookup table with normal dot light on the horizontal axis
         * and normal dot view on the vertical axis, taking into account the shadow
         * attenuation by taking the min of the shadow attenuation and N dot L.
         *
         * @param NoV               dot of the normal and view, should not be saturated or abs'd (get it from fragData not surfData)
         * @param NoL               dot of the normal and light, should not be saturated or abs'd
         * @param shadowAttenuation shadow attenuation value associated with the light
         * @return BRDF color for the given dot products of the light, normal, and view direction
         */
        half3 SampleBDRFLUTShadow( half NoV, half NoL, half shadowAttenuation)
        {
            half NoL2 = saturate((NoL + 1) * 0.5);
            half shadowGamma2 = sqrt(shadowAttenuation); 
            half clampedNoL2 = min(NoL2, shadowGamma2);
            return this.BRDFLut.SampleLevel(sampler_LinearClamp, half2(clampedNoL2, saturate(NoV)), 0).rgb;
        }

        /**
         * Calculates the diffuse light radiance for the given fragment and light data using a BRDF lookup table
         * to adjust the light color and intensity
         *
         * @param md    Struct containing properties related to the interpolated vertex data at the fragment
         * @param ps    Struct containing physical properties of the surface at the fragment
         * @param light Struct containing information about the light
         * @return Outgoing diffuse light intensity not yet modulated by the albedo
         */
        LIGHT_VEC PunctualDiffuse(LightMeshData md, LightPhysData ps, Light light)
        {
            half3 brdfLut = SampleBDRFLUTShadow(md.NoV, dot(md.normal, light.direction), light.shadowAttenuation);
            LIGHT_VEC attenuatedLight = (LIGHT_VEC)light.color * (light.distanceAttenuation * light.shadowAttenuation);
            attenuatedLight.rgb *= brdfLut;
            return attenuatedLight;
        }

        /**
         * Calculates the diffuse light radiance for the given fragment and spherical harmonic data, faking subsurface scattering
         * by bending the normal direction used to calculate each color channel toward the dominant light direction based on
         * the values in SubSurfScatterIntensity
         *
         * @param md    Struct containing properties related to the interpolated vertex data at the fragment
         * @param ps    Struct containing physical properties of the surface at the fragment
         * @param sh    Struct containing the spherical harmonic coefficents
         * @return Outgoing diffuse light intensity not yet modulated by the albedo
         */
        half3 ShDiffuse(LightMeshData md, LightPhysData ps, ShCoefficients sh)
        {

            // Soften N toward the L1-derived ambient gather direction
            // Larger softening factor = larger wraparound (deeper scatter)
            // Dominant direction from luminance-summed L1
            half3 l1Vec = sh.L1L0r.xyz + sh.L1L0g.xyz + sh.L1L0b.xyz;
            float  l1LenSq = max(dot(l1Vec, l1Vec), SAFE_FLT_RSQRT_MIN);
            half3  dominantDir = (half3)(l1Vec * rsqrt(l1LenSq));
            
            // Soften normal per channel — red scatters deepest, blue not at all.
            // Tune these to taste / expose as material params.
            half3 nR = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.r));
            half3 nG = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.g));
            half3 nB = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.b));

            half3 result = half3(
                            dot(nR, sh.L1L0r.rgb) + sh.L1L0r.a,
                            dot(nG, sh.L1L0g.rgb) + sh.L1L0g.a,
                            dot(nB, sh.L1L0b.rgb) + sh.L1L0b.a
                        );
            #if !defined(PROBE_VOLUMES_L1) && !defined(EVALUATE_SH_MIXED)
                result += half3(
                    SHEvalLinearL2Channel(nR, sh.L2Br, sh.L2C.r),
                    SHEvalLinearL2Channel(nG, sh.L2Bg, sh.L2C.g),
                    SHEvalLinearL2Channel(nB, sh.L2Bb, sh.L2C.b)
                );
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

            #if defined(DIRLIGHTMAP_MONOSH)
                half3 L1Mono = 4.0 * dirLmTexel.xyz - 2.0;
                half3 dominantDir = normalize(L1Mono);
                half3 nR = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.r));
                half3 nG = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.g));
                half3 nB = SafeNormalize(lerp(md.normal, dominantDir, this.SubSurfScatterIntensity.b));
                L1Mono *= lightmapTexel;
                result = half3(
                    dot(L1Mono, nR) + lightmapTexel.r,
                    dot(L1Mono, nG) + lightmapTexel.g,
                    dot(L1Mono, nB) + lightmapTexel.b,
                ); 
            #else //!DIRLIGHTMAP_MONOSH
                half3 lightmapDir = dirLmTexel.xyz;
                half halfLambert = (dot(md.normal, 0.5h * lightmapDir) + half(0.5)) / max(half(1e-4), dirLmTexel.w);
                half3 brdfLUT = this.BRDFLut.SampleLevel(sampler_LinearClamp, half2(halfLambert, saturate(NoV)), 0).rgb;
                result = lightmapTexel * brdfLUT;
            #endif //!DIRLIGHTMAP_MONOSH 

            #endif // DIRLIGHTMAP_COMBINED

            return result;
        }
    };

#if !defined(TEMPLATES_SUPPORTED)

    #define DIFFUSE_MODEL DiffuseModelSkinBrdfLut
    #define MESH_DATA LightMeshData
    #define PHYS_DATA LightPhysData
    #define SPECULAR_MODEL SpecularModelKSK
    
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
    
    #undef MESH_DATA
    #undef PHYS_DATA
    #undef SPECULAR_MODEL


    #define DIFFUSE_MODEL DiffuseModelSkinBrdfLut
    #define MESH_DATA LightMeshData
    #define PHYS_DATA LightPhysData
    #define SPECULAR_MODEL SpecularModelGGX
    
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/Templates/PhysicallyBasedLighting.hlsl"
    
    #undef MESH_DATA
    #undef PHYS_DATA
    #undef SPECULAR_MODEL

#endif // !TEMPLATES_SUPPORTED
}
#endif
