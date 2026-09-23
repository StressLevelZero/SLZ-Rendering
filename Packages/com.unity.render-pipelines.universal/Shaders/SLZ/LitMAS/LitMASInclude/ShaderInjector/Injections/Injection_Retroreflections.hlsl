//#!INJECT_BEGIN UNIVERSAL_DEFINES 0
#define _RETROREFLECTIVE
//#!INJECT_END

//#!INJECT_BEGIN INCLUDES 0
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/LightModelRetroRefl.hlsl"
//#!INJECT_END

//#!INJECT_BEGIN UNIFORMS 0
TEXTURE2D(_RetroReflMap);
//#!INJECT_END

//#!INJECT_BEGIN PHYSDATA_TYPE 0
	SLZ::LightPhysDataRetroRefl physData;
//#!INJECT_END

//#!INJECT_BEGIN PHYSDATA_POPULATE_EXTRA 10
	physData.retroReflPercent = _RetroReflIntensity * SAMPLE_TEXTURE2D(_RetroReflMap, sampler_BaseMap, uv0).r;
	physData.retroReflSharpness = _RetroReflSharpness;
//#!INJECT_END

//#!INJECT_BEGIN LIGHTING_CALC 0
	color = SLZ::PhysicallyBasedLighting(meshData, physData, (SLZ::SpecularModelRetroRefl)0, (SLZ::DiffuseModelRetroRefl)0);
//#!INJECT_END