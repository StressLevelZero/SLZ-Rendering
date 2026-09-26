#if !defined(LIGHT_MODEL_RETRO_REFL_INCLUDED)
#define LIGHT_MODEL_RETRO_REFL_INCLUDED

namespace SLZ
{
    struct LightPhysDataRetroRefl : LightPhysData
    {
        half retroReflSharpness;
        half retroReflPercent;
    };

    /** @brief Lobe used to fake a retro-reflective effect for diffuse surfaces. Not physically based.
     *      Uses the Lagrange GGX NDF since it works with fp16 and integrates to 1 over the sphere.
     *
     * @param viewDir   normalized fragment to camera vector
     * @param lightDir  normalized fragment to light vector
     * @param sharpness How sharp is the lobe (ie how focused is the retroreflector)
     *
     * @returns lobe used to determine the amount of light reflected back towards the view direction
     */
    half RetroreflectionLobe(half3 normal, half3 viewDir, half3 lightDir, half sharpness)
    {
        #if 1
        LagrangeGGXParams ggxParams = LagrangeGGXParams(lightDir, viewDir, lightDir);
        half NoL = saturate(dot(normal, lightDir)); 
        half retroLobe = NoL > 0 ? NoL * GgxNdfLagrange(ggxParams.NoH, ggxParams.NxH2, 1.0 - sharpness) : 0;
        return retroLobe;
        #else
        // Von-mises fisher, way more expensive and suffers precision issues with fp16 at high sharpness
        half roughness = half(1.0) - sharpness;
        half invLambda = 0.5 * roughness * roughness;
        float exp2L = exp( -2.0 / invLambda );
        float v = 1.0 / (invLambda * 2 * PI * (1.0 - exp2L)) * exp((1.0 / invLambda) * (dot(viewDir, lightDir) - 1.0));
        return v;
        #endif
    }

    // Model retroreflecive surface as 3 layers: outer surface that produces typical specular reflections, middle layer that contains ordered
    // facets that reflect light mostly back in the direction it came from via total internal reflection, and back layer that scatters light 
    // according to lambert diffuse. Middle layer is assumed to be partially transparent. Light passing into the middle layer is tinted by the
    // albedo color. The easiest way to implement the middle retroreflective layer is to integrate it into the diffuse functions so it can reduce
    // the amount of light reaching the final diffuse layer. Since it should be multiplied by the albedo, we should be able to directly add it to
    // the diffuse light contribution.
    class DiffuseModelRetroRefl
    {

        #if !defined(HLSL_2021)
        uint disambiguationRetro1;
        min16uint disambiguationRetro2;
        #endif
        
        static LIGHT_VEC PunctualDiffuse(LightMeshData md, LightPhysDataRetroRefl ps, Light light)
        {
            LIGHT_VEC lambertDiffuse = DiffuseModelLambert::PunctualDiffuse(md, ps, light);
            half retroReflLobe = RetroreflectionLobe(md.normal, md.viewDir, light.direction, ps.retroReflSharpness);
            retroReflLobe = min(half(100.0), retroReflLobe); // prevent +inf when viewDir == -light.direction
            LIGHT_VEC retroReflLight = saturate(md.NoV * md.NoV) * retroReflLobe * light.distanceAttenuation * light.shadowAttenuation * light.color;
            return (ps.retroReflPercent * retroReflLight) + ((half(1.0) - ps.retroReflPercent) * lambertDiffuse);
        }

        static half3 ShDiffuse(LightMeshData md, LightPhysDataRetroRefl ps, ShCoefficients sh)
        {
            half3 fakeLightDirection = ShFakeSpecularDirection(sh);
           
            half3 retroreflectionEnergy = half3(
                dot(fakeLightDirection, sh.L1L0r.xyz),
                dot(fakeLightDirection, sh.L1L0g.xyz),
                dot(fakeLightDirection, sh.L1L0b.xyz)); 

            half retroReflLobe = RetroreflectionLobe(md.normal, md.viewDir, fakeLightDirection, ps.retroReflSharpness);
            half3 retroReflLight = ps.retroReflPercent * saturate(md.NoV * md.NoV) * retroReflLobe * retroreflectionEnergy;

            half3 lambertDiffuse = DiffuseModelLambert::ShDiffuse(md, ps, sh);
            lambertDiffuse *= half(1.0) - ps.retroReflPercent;
            
            return retroReflLight + lambertDiffuse;
        }

        static half3 LightmapDiffuse(LightMeshData md, LightPhysDataRetroRefl ps, half3 lightmapTexel, half4 dirLmTexel)
        {
            return DiffuseModelLambert::LightmapDiffuse(md, ps, lightmapTexel, dirLmTexel);
        }
    };

    class SpecularModelRetroRefl
    {
        #if !defined(HLSL_2021)
        bool disambiguationKSK;
        min16uint disambiguationRetro2;
        #endif

        static half3 ReflectionVector(LightMeshData md, LightPhysData ps)
        {
            return ReflectionDirIso(md.viewDir, md.normal);
        }
        
        static half3 PunctualSpecular(LightMeshData md, LightPhysDataRetroRefl ps, half3 lightDir, half2 fgd)
        {
           return SpecularModelKSK::PunctualSpecular(md, ps, lightDir, fgd);
        }

        static half3 PunctualSpecular(LightMeshData md, LightPhysData ps, Light light, half2 fgd)
        {
            return SpecularModelKSK::PunctualSpecular(md, ps, light, fgd);
        }

        // Hijack IBL specular reflections for retroreflections so we don't sample the IBL lighting twice
        static half3 IblSpecular(LightMeshData md, LightPhysDataRetroRefl ps, half3 reflectionDir, half2 fgd)
        {
            half lerpFactor = smoothstep(half(0.1), half(0.3), ps.retroReflPercent);
            half roughness = lerp(ps.Roughness(), (half(1.0) - ps.retroReflSharpness), lerpFactor);
            half3 specularF0 = lerp(ps.SpecularF0(), md.NoV * md.NoV * ps.AlbedoAlpha().rgb, lerpFactor);
            fgd = lerp(fgd, half2(0,1), lerpFactor);
            // Lerp has artifacting on the x/y/z planes, but doing the (presumably) same math doesn't?
            reflectionDir = reflectionDir * (half(1.0) - lerpFactor) + md.viewDir * lerpFactor;
            return 
                ProbeIblSpecularMultiscatterFGD(reflectionDir, md.positionWS, roughness, md.screenUV, md.NoV, specularF0, fgd) 
                * SpecularHorizonOcclusion(md.normal, md.meshNormal, reflectionDir)
                ;
        }

        static half3 ShFakeSpecular(LightMeshData md, LightPhysData ps, ShCoefficients sh, half3 shDiffuse, half2 fgd)
        {
            return SpecularModelKSK::ShFakeSpecular(md, ps, sh, shDiffuse, fgd);
        }

        static half3 LightmapFakeSpecular(LightMeshData md, LightPhysData ps, half3 lightmapIrradiance, half4 dirLightmap, half2 fgd)
        {
             return SpecularModelKSK::LightmapFakeSpecular(md, ps, lightmapIrradiance, dirLightmap, fgd);
        }
    };
}

#endif
