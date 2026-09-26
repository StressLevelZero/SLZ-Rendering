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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Premultiply Alpha
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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

    LIGHT_VEC diffuse = (LIGHT_VEC)0;
    diffuse.rgb = meshData.vertexLighting;
    half3 specular = 0;
    half4 shadowMask = (half4)0;
    half2 FGD = SampleFgd(_FgdGgx, saturate(meshData.NoV), saturate(physData.PerceptualRoughness()));

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Image-based specular
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

    half3 reflectionDir = specularModel.ReflectionVector(meshData, physData);
    specular += specularModel.IblSpecular(meshData, physData, reflectionDir, FGD);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Sample Lightmap or Light Probes
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
    #if defined(LIGHTMAP_ON) || defined(DIRLIGHTMAP_COMBINED)
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

        diffuse.rgb += diffuseModel.LightmapDiffuse(meshData, physData, lightmapTexel, dirLmTexel) * physData.occlusion;
        #if defined(DIRLIGHTMAP_COMBINED)
            specular += specularModel.LightmapFakeSpecular(meshData, physData, diffuse, dirLmTexel, FGD);
        #endif
    #else // Light probes
        ShCoefficients shCoeff = SphericalHarmonicCoefficients(meshData.positionWS, meshData.normal, meshData.viewDir, 0);
        #if defined(SHADOWS_SHADOWMASK)
            shadowMask = shCoeff.probeOcclusion;
        #endif
        diffuse.rgb += diffuseModel.ShDiffuse(meshData, physData, shCoeff) * physData.occlusion;
        specular += specularModel.ShFakeSpecular(meshData, physData, shCoeff, diffuse, FGD);
    #endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Sample Main Light
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

    #if defined(_LIGHT_LAYERS)
        uint meshRenderingLayers = GetMeshRenderingLayer();
    #endif

    Light mainLight = GetMainLight(meshData.shadowCoord, meshData.positionWS, shadowMask);

    #if defined(_LIGHT_LAYERS)
        if (IsMatchingLightLayer(mainLight.layerMask, meshRenderingLayers))
    #endif
    if (any(mainLight.color > half(0.0)))
    {
        diffuse += diffuseModel.PunctualDiffuse(meshData, physData, mainLight);
        specular += specularModel.PunctualSpecular(meshData, physData, mainLight, FGD);
    }
    
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Sample Additional lights
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

    // Mobile fixed address lights. Unity provides a large global array of 
    // lights and a per-object float4[2] containing indices in that array for 
    // the current draw. Qualcomm Adreno hardware performs suprisingly poorly
    // when doing this, it is recommended that array indices are literals
    // known at compile time otherwise the driver turns them into GMEM fetches
    // which essentially have the overhead of a texture sample. Workaround
    // is to provide a very small set of global lights and blindly evalute
    // them in order regardless of if they affect the draw or not.
#if defined(SLZ_FIXED_ADDRESS_LIGHTS)

    for (int lIdx = 0; lIdx < MAX_FIXED_ADDRESS_LIGHTS; lIdx++)
    {
        if (lIdx >= _FixedLightCount) break;
        Light fixedLight = GetFixedAddressAdditionalLight(lIdx, meshData.positionWS);
        diffuse += diffuseModel.PunctualDiffuse(meshData, physData, fixedLight);// * fixedLight.distanceAttenuation;// * light.shadowAttenuation;
        specular += specularModel.PunctualSpecular(meshData, physData, fixedLight, FGD);
    }
#elif defined(_ADDITIONAL_LIGHTS) // Normal Unity forward lights

    uint pixelLightCount = GetAdditionalLightsCount();

    // Forward+ has additional directional lights
    #if USE_CLUSTER_LIGHT_LOOP
    [loop] for (uint lightIndex = 0; lightIndex < min(URP_FP_DIRECTIONAL_LIGHTS_COUNT, MAX_VISIBLE_LIGHTS); lightIndex++)
    {
        
        Light light = GetAdditionalLight(lightIndex, meshData.positionWS, shadowMask);

        #ifdef _LIGHT_LAYERS
        if (IsMatchingLightLayer(light.layerMask, meshRenderingLayers))
        #endif
        {
            diffuse += diffuseModel.PunctualDiffuse(meshData, physData, light);
            specular += specularModel.PunctualSpecular(meshData, physData, light, FGD);
        }
    }
    #endif // end USE_CLUSTER_LIGHT_LOOP

    // Fix Unity's light loop macros assuming URP Lit's input struct
    #define inputData               meshData
    #define normalizedScreenSpaceUV screenUV

    LIGHT_LOOP_BEGIN(pixelLightCount)
        Light light = GetAdditionalLight(lightIndex, meshData.positionWS, shadowMask);

        if (light.distanceAttenuation > 0.0
            #ifdef _LIGHT_LAYERS
            && IsMatchingLightLayer(light.layerMask, meshRenderingLayers)
            #endif
            )
        {
            
            diffuse += diffuseModel.PunctualDiffuse(meshData, physData, light);
            specular += specularModel.PunctualSpecular(meshData, physData, light, FGD);
        }
    LIGHT_LOOP_END
    #undef inputData                        
    #undef normalizedScreenSpaceUV
#endif // end _ADDITIONAL_LIGHTS

    half4 output = (half4)0;
#if defined(SLZ_FLUORESCENCE)
    output.rgb += Fluorescence(diffuse, physData.GetFluorescentAbsorbance(), physData.GetFluorescentColor()); 
#endif
    output += half4(diffuse.rgb * physData.AlbedoAlpha().rgb + specular, 1);
    output.rgb += physData.emission;
    return output;
}

#endif // end SLZ_PBR_LIGHTING_TEMPLATE