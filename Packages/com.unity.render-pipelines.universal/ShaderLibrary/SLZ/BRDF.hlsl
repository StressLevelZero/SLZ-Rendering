#ifndef SLZ_BRDF_INCLUDED
#define SLZ_BRDF_INCLUDED

#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl"
#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Macros.hlsl"

namespace SLZ
{
    ///----------------------------------------------------------------------
    ///----------------------------------------------------------------------
    /// Default (Isotropic) Specular BRDF 
    ///----------------------------------------------------------------------
    ///----------------------------------------------------------------------
    
    /** 
     * Parameters for GGX Specular, using Lagrange's identity to replace the 1 - NoH^2 term in the NDF
     * with cross(N, H)^2 as suggested by Google's Filament renderer. The behavior of the cross product
     * is far more favorable with 16 bit precision, fixing severe rounding errors when NoH is close to 0 
     * that occurs with the unmodified formula.
     */

    struct LagrangeGGXParams
    {
        half NoH;
        half LoH;
        half NxH2;
        half NoL;

        /**
         * Constructor for LagrangeGGXParams
         *
         * @param normal   Worldspace normal of the fragment
         * @param viewDir  Normalized vector from the fragment to the camera, in worldspace
         * @param lightDir Normalized vector from the fragment to the light source, in worldspace
         */
        static LagrangeGGXParams ctor(const half3 normal, const half3 viewDir, const half3 lightDir)
        {
            LagrangeGGXParams ggxParams = (LagrangeGGXParams) 0;
            half3 halfDir = SafeNormalize(lightDir + viewDir);
            ggxParams.NoH = saturate(dot(normal, halfDir));
            ggxParams.LoH = saturate(dot(lightDir, halfDir));

            // See https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/normaldistributionfunctionspeculard
            // Qualcomm Adreno drivers started severely rounding the result of a cross of half-vectors. Components smaller than 2^-7 get rounded down to 0, so when the normal and half vectors are close it rounds to 0. Makes the NDF turn into a sharp square :(. 
            // Stupid solution: multiply the normalized half-vector by 4. The cross product of the 4x half-vector and the normal vector is also 4x as long, and thus the normal and half vectors can get far closer before the components of the result round to 0 
            // After dotting the 4x cross vector with itself, we can divide by 16 to get the actual value
    
            half3 NxH = cross(normal, half(4.0) * halfDir);
            ggxParams.NxH2 = saturate(dot(NxH, NxH) * half(0.0625h));
            ggxParams.NoL = dot(normal, lightDir);
            return ggxParams;
        }
    };
    #define LagrangeGGXParams(a,b,c) LagrangeGGXParams::ctor(a,b,c)


    /** 
     * GGX normal distribution function optimized for half-precision. 
     * Uses Lagrange's identity (dot(cross(A, B), cross(A, B)) = length(A)^2 * length(B)^2 - dot(A, B)^2)
     * to avoid calculating 1 - dot(N, H)^2 which has severe precision issues when dot(N, H) is close to 1
     * See the Google Filament documentation https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf  
     *
     * @param NoH       Dot product of the normal with half view-light vector
     * @param NxH2      Cross-product of the normal and half view-light, dotted with itself  
     * @param roughness Non-perceptual roughness value
     * @return GGX normal distribution value
     */
    half GgxNdfLagrange(half NoH, half NxH2, half roughness)
    {
        half a = NoH * roughness;
        half d = roughness / max(a * a + NxH2, half(HALF_MIN));
        half d2 = (d * d * half(INV_PI));
        return d2;
    }

    /** 
     * Original GGX normal distribution function for full 32 bit float precision
     *
     * @param NoH       Dot product of the normal with half view-light vector
     * @param roughness Non-perceptual roughness value
     * @return GGX normal distribution value
     */
    float GgxNdfFp32(float NoH, float roughness)
    {
        float a = NoH * roughness;
        float d = roughness / (a * a - NoH * NoH + 1.0);
        float d2 = (d * d * INV_PI);
        return d2;
    }

    /**
     * Kelemen and Szirmay-Kalos (KSK) visibility with J. Hable's roughness term, acts as both the visibility and fresnel functions
     * This is unity's default for the URP. Extremely cheap, perfect for mobile.
     * See SIGGRAPH 2015 "Optimizing PBR" for more details (https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_renaldas_2D00_slides.pdf)
     *
     * @param LoH         Dot product of the light direction with the half view-light vector
     * @param roughness   Surface roughness (non-perceptual)
     * @return pre-multiplied geometic shadowing and fresnel terms of the specular BRDF
     */
    half FusedVisFresnel(half LoH, half roughness)
    {
        half LoH2 = LoH * LoH;
        return rcp(max(half(0.1), LoH2) * (half(4.0) * roughness + half(2.0)));
    }


    /** 
     * Heitz height-correlated Smith-GGX visibility function (specular geometric shadowing)
     *
     * @param NoV       Normal-view dot product
     * @param NoL       Normal-light dot product
     * @param roughness Non-perceptual roughness
     * @return Geometric shadowing term of the specular BRDF
     */
    half SmithVisibility(half NoV, half NoL, half roughness)
    {
        NoV = abs(NoV) + half(1e-5);
        NoL = abs(NoL) + half(1e-5);
        half rough2 = roughness * roughness;
        half v = NoL * sqrt(NoV * (-rough2 * NoV + half(1.0)) + rough2);
        half l = NoV * sqrt(NoL * (-rough2 * NoL + half(1.0)) + rough2);
        return half(0.5) / max(v + l, HALF_MIN);
    }

    /** 
     * Schlick Fresnel function
     *
     * @param LoH           Dot product of light direction with half light-view vector  
     * @param reflectance0  Specular reflectance at normal incidence
     * @param reflectance90 Specular reflectance at grazing incidence
     */ 
    half3 SchlickFresnel(const half LoH, const half3 reflectance0, const half3 reflectance90)
    {
        half iLoH = half(1.0) - LoH;
        half iLoH5 = pow(iLoH, half(5.0));
        return reflectance0 + (reflectance90 - reflectance0) * iLoH5;
    }

    /** Punctual specular using float16-optimized GGX normal distibution function and KSK fused visibility and fresnel.
     *
     * @param ggx         Specular parameters
     * @param roughness   non-perceptual roughness
     * @param reflectance Specular reflectance at normal incidence
     */
    half3 SpecBrdfFp16KSK(const LagrangeGGXParams ggx, half roughness, const half3 reflectance)
    {
        roughness = half(0.999) * roughness + half(0.001); // remap to [0.001,1] to prevent specular aliasing
        half NDF = GgxNdfLagrange(ggx.NoH, ggx.NxH2, roughness);
        half VF = FusedVisFresnel(ggx.LoH, roughness);
        half specularTerm = (NDF * VF);
    // Hack added in unity's original BRDF, apparently compiler will optimize clamp to min since it thinks
    // that specularTerm can never be <0, but float16 can overflow to negative values.
        specularTerm = specularTerm - HALF_MIN;
        specularTerm = clamp(specularTerm, half(0.0), half(100.0)); // Prevent FP16 overflow
        return specularTerm * reflectance;
    }

    /** Punctual specular using float16-optimized GGX normal distibution function, and the normal Smith visibility and Schlick fresnel.
     *
     * @param ggx           Specular parameters
     * @param NoV           dot product of the normal and view directions
     * @param roughness     non-perceptual roughness
     * @param reflectance0  Specular reflectance at normal incidence
     * @param reflectance90 (Optional) Specular reflectance at grazing incidence. Default is (1,1,1)
     */
    half3 SpecBrdfFp16(const LagrangeGGXParams ggx, half NoV, half roughness, const half3 reflectance0, const half3 reflectance90 = half3(1, 1, 1))
    {
        roughness = half(0.999) * roughness + half(0.001); // remap to [0.001,1] to prevent specular aliasing
        half N = GgxNdfLagrange(ggx.NoH, ggx.NxH2, roughness);
        half D = SmithVisibility(NoV, ggx.NoL, roughness);
        half3 F = SchlickFresnel(ggx.LoH, reflectance0, reflectance90);
    
        half3 specularTerm = (N * D * F) - HALF_MIN;
        specularTerm = clamp(specularTerm, 0.0, 100.0);
    
        return specularTerm;
    }
    
    /**
     * Lambert diffuse, simplest diffuse BDRF possible
     *
     * @param light           Incoming light intensity
     * @param normal          Worldspace normal
     * @param lightDir        Unit vector pointing from the fragment to the light in worldspace
     */
    half3 LambertDiffuse(const half3 light, const half3 normal, const half3 lightDir)
    {
        return light * saturate(dot(normal, lightDir));
    }

    ///----------------------------------------------------------------------
    ///----------------------------------------------------------------------
    /// Anisotropic Specular BRDF 
    ///----------------------------------------------------------------------
    ///----------------------------------------------------------------------

    struct AnisoGGXParams
    {
        half NoH2;
        half NoL;
        half LoH;
        half ToL;
        half BoL;
        half ToH;
        half BoH;

        static AnisoGGXParams ctor(const half3 normal, const half3 tangent, const half3 bitangent, const half3 viewDir, const half3 lightDir)
        {
            AnisoGGXParams ggxParams = (AnisoGGXParams) 0;
            half3 halfDir = SafeNormalize(lightDir + viewDir);

            half3 NxH = cross(normal, halfDir);
            ggxParams.NoH2 = half(1.0) - dot(NxH, NxH);
            ggxParams.ToH = dot(tangent, halfDir);
            ggxParams.BoH = dot(bitangent, halfDir);

            ggxParams.LoH = saturate(dot(lightDir, halfDir));
            ggxParams.ToL = dot(tangent, lightDir);
            ggxParams.BoL = dot(bitangent, lightDir);
            ggxParams.NoL = saturate(dot(normal, lightDir));
        
            return ggxParams;
        }
    };
    #define AnisoGGXParams(n,t,b,v,l) AnisoGGXParams::ctor(n,t,b,v,l)

}

#endif
