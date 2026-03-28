#ifndef SLZ_PBR_LIGHTING_TEMPLATE

#ifdef TEMPLATES_SUPPORTED
    // only include this file once if templates are supported
    #define SLZ_PBR_LIGHTING_TEMPLATE 
    template<typename MESH_DATA, typename PHYS_DATA, typename SPECULAR_MODEL>
#endif
half4 PhysicallyBasedLighting(MESH_DATA meshData, PHYS_DATA physData, SPECULAR_MODEL specularModel)
{

    // handle alpha premultiplication for transparent surfaces. diffuse lighting is multiplied by alpha, but specular is unaffected.
    // Instead, reflectivity reduces transmission of light.  
    if (physData.surfaceType == SurfaceType::Transparent)
    {
        physData.albedo *= physData.alpha;
        physData.emission *= physData.alpha;

        // increase the alpha to 1 as the specular reflectance goes to 1
        half3 normRefl = physData.NormalSpecReflectance();
        half monoReflectance = max(normRefl, max(normRefl, normRefl));
        // inaccurate pow4 fresnel, but good enough
        half fresnelTerm = (half(1.0h) - saturate(meshData.NoV));
        fresnelTerm *= fresnelTerm;
        fresnelTerm *= fresnelTerm;
        physData.alpha = saturate(physData.alpha + lerp(monoReflectance, 1, fresnelTerm));
    }

    half3 diffuse = meshData.vertexLighting * physData.albedo;

    // TODO: do we bake in the reflectionDir calculation into CalculateIBLSpecular, or separate it so other functions can access it?
    half3 reflectionDir = SPECULAR_MODEL::CalculateReflectionVector(meshData, physData);
    half3 specular = SPECULAR_MODEL::CalculateIBLSpecular(meshData, physData, reflectionDir);

    //specular += SPECULAR_MODEL::CalculatePunctualSpecular(meshData, physData, half3(0, 1, 0));

    return half4(specular, 1);
}

#endif // SLZ_PBR_LIGHTING_TEMPLATE
