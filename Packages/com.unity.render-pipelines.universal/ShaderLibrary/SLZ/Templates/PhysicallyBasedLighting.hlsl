#ifndef SLZ_PBR_LIGHTING_TEMPLATE

#ifdef TEMPLATES_SUPPORTED
    // only include this file once if templates are supported
    #define SLZ_PBR_LIGHTING_TEMPLATE 
#endif


#ifdef TEMPLATES_SUPPORTED
    template<typename MESH_DATA, typename PHYS_DATA, typename SPECULAR_MODEL, typename DIFFUSE_MODEL>
#endif
half4 PhysicallyBasedLighting(MESH_DATA meshData, PHYS_DATA physData, SPECULAR_MODEL specularModel, DIFFUSE_MODEL diffuseModel)
{

    // handle alpha premultiplication for transparent surfaces. diffuse lighting is multiplied by alpha, but specular is unaffected.
    // Instead, reflectivity reduces transmission of light.  
    if (physData.surfaceType == SurfaceType::Transparent)
    {
        half4 albedoAlpha = physData.AlbedoAlpha();
        physData.emission *= albedoAlpha.a;
        // increase the alpha to 1 as the specular reflectance goes to 1
        half3 normRefl = physData.SpecularF0();
        half monoReflectance = max(normRefl.x, max(normRefl.y, normRefl.z));
        // inaccurate pow4 fresnel, but good enough
        half fresnelTerm = (half(1.0h) - saturate(meshData.NoV));
        fresnelTerm *= fresnelTerm;
        fresnelTerm *= fresnelTerm;
        half newAlpha = saturate(albedoAlpha.a + lerp(monoReflectance, 1, fresnelTerm));
        physData.SetAlbedoAlpha(albedoAlpha.rgb * albedoAlpha.a, newAlpha);
    }

    half3 diffuse = 0;
    half3 specular = 0;
    half2 FGD = SampleFgd(_FgdGgx, saturate(meshData.NoV), saturate(physData.PerceptualRoughness()));

    half3 reflectionDir = specularModel.ReflectionVector(meshData, physData);
    specular += specularModel.IblSpecular(meshData, physData, reflectionDir, FGD);

    half4 shadowMask = (half4)0;

    // Sample Lightmap or Light Probes
    #if defined(LIGHTMAP_ON)
        const bool lmBicubicSample = 
        #if defined(LIGHTMAP_BICUBIC_SAMPLING)
            true;
        #else
            false;
        #endif

        half3 lightmapTexel = SampleLightmapTexture(LIGHTMAP_NAME, LIGHTMAP_SAMPLER_NAME, meshData.lightmapUV, lmBicubicSample).rgb;

        half4 dirLmTexel =
        #if defined(DIRLIGHTMAP_COMBINED)
             SampleLightmapTexture(LIGHTMAP_INDIRECTION_NAME, LIGHTMAP_SAMPLER_NAME, meshData.lightmapUV, lmBicubicSample);
        #else
            (half4)0;
        #endif

        diffuse += diffuseModel.LightmapDiffuse(meshData, physData, lightmapTexel, dirLmTexel);
        #if defined(DIRLIGHTMAP_COMBINED)
            specular += specularModel.LightmapFakeSpecular(meshData, physData, diffuse, dirLmTexel, FGD);
        #endif
    #else
        ShCoefficients shCoeff = SphericalHarmonicCoefficients(meshData.position, meshData.normal, meshData.viewDir, 0);
        #if defined(SHADOWS_SHADOWMASK)
        shadowMask = shCoeff.probeOcclusion;
        #endif
        diffuse = diffuseModel.ShDiffuse(meshData, physData, shCoeff);
        specular += specularModel.ShFakeSpecular(meshData, physData, shCoeff, diffuse, FGD);
        
    #endif


    return half4(diffuse * physData.AlbedoAlpha().rgb + specular, 1);
}

#endif // SLZ_PBR_LIGHTING_TEMPLATE
