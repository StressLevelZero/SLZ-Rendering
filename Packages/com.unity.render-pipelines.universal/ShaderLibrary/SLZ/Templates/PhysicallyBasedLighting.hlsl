#ifndef SLZ_PBR_LIGHTING_TEMPLATE

#ifdef TEMPLATES_SUPPORTED
    // only include this file once if templates are supported
    #define SLZ_PBR_LIGHTING_TEMPLATE 
    template<typename MESH_DATA, typename PHYS_DATA, typename SPECULAR_MODEL>
#endif
half4 PhysicallyBasedLighting(MESH_DATA meshData, PHYS_DATA physData, SPECULAR_MODEL specularModel)
{

    // handle alpha premultiplication for transparent surfaces. diffuse lighting is darkened by alpha, but specular is unaffected.
    // Instead, reflectivity reduces transmission of light 
    if (physData.surfaceType == min16int(1))
    {
        physData.albedo *= physData.alpha;
        physData.emission *= physData.alpha;


        half monoReflectivity = max(physData.specularColor.r, max(physData.specularColor.g, physData.specularColor.b));
        half fresnelTerm = (half(1.0h) - saturate(meshData.NoV));
        fresnelTerm *= fresnelTerm;
        fresnelTerm *= fresnelTerm;
        physData.alpha = lerp(monoReflectivity, 1, fresnelTerm);
    }

    half3 diffuse = meshData.vertexLighting * physData.albedo;

    half3 specular = half3(0.0h, 0.0h, 0.0h);



    return half4(SPECULAR_MODEL::CalculatePunctualSpecular(meshData, physData, half3(0, 1, 0)), 1);
}

#endif // SLZ_PBR_LIGHTING_TEMPLATE
