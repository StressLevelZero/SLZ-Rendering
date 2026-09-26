//#!INJECT_BEGIN INCLUDES 0
#if defined(_BRDFMAP)
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/LightingModelBrdfLut.hlsl"
#endif
//#!INJECT_END

//#!INJECT_BEGIN MATERIAL_CBUFFER_HALF_VECTORS 0
half4  _SSSColor;
//#!INJECT_END

//#!INJECT_BEGIN UNIFORMS 0
TEXTURE2D(g_tBRDFMap);
//#!INJECT_END


//#!INJECT_BEGIN LIGHTING_CALC 0
#if defined(_BRDFMAP)
    SLZ::DiffuseModelSkinBrdfLut diffuseModel;
    diffuseModel.BRDFLut = g_tBRDFMap;
    diffuseModel.SubSurfScatterIntensity = _SSSColor;
	color = SLZ::PhysicallyBasedLighting(meshData, physData, (SLZ::SpecularModelKSK)0, diffuseModel);
#else
    color = SLZ::PhysicallyBasedLighting(meshData, physData, (SLZ::SpecularModelKSK)0, (SLZ::DiffuseModelLambert)0);
#endif
//#!INJECT_END