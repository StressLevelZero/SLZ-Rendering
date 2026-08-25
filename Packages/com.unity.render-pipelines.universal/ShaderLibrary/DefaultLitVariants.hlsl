#pragma once
#warning USING LEGACY SHADERINCLUDE DefaultLitVariants.hlsl! DO NOT USE IN NEW SHADERS!

#if !defined(_DISABLE_VOLUMETRICS)
	#if defined(SHADER_API_MOBILE)
		#pragma multi_compile_fragment _ _VOLUMETRICS_ENABLED
	#else
		#pragma multi_compile_fragment _ _VOLUMETRICS_ENABLED_HQ _VOLUMETRICS_ENABLED
		
		#if defined(_VOLUMETRICS_ENABLED_HQ)
			#define(_VOLUMETRICS_ENABLED)
		#endif
	#endif
#endif

//#pragma multi_compile_fragment _ _VOLUMETRICS_ENABLED
//#pragma multi_compile_fog
//#pragma skip_variants FOG_LINEAR FOG_EXP
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Fog.hlsl"

#if !defined(SHADER_API_MOBILE)
//#pragma multi_compile _FORWARD_PLUS
#endif

#if defined(SHADER_API_MOBILE)
	#define _ADDITIONAL_LIGHTS_VERTEX 1
	#pragma multi_compile _ _REFLECTION_PROBE_BOX_PROJECTION
	// wait on this until DXC gets multiview stereo support
	//#pragma multi_compile _ _REFLECTION_PROBE_BLENDING
#else
	#pragma multi_compile_fragment _ _MAIN_LIGHT_SHADOWS_CASCADE
	#define _SHADOWS_SOFT 1

	#if !defined(_DISABLE_ADDLIGHTS)

		#define DYNAMIC_ADDITIONAL_LIGHTS 1
		#pragma dynamic_branch _ADDITIONAL_LIGHTS


		#define DYNAMIC_ADDITIONAL_LIGHT_SHADOWS 1
		#pragma dynamic_branch _ADDITIONAL_LIGHT_SHADOWS

	#endif

	
	#define _REFLECTION_PROBE_BLENDING 1
	#define _REFLECTION_PROBE_BOX_PROJECTION 1

	
	#if !defined(_DISABLE_SSAO)

		#define DYNAMIC_SCREEN_SPACE_OCCLUSION
		#pragma dynamic_branch_fragment _SCREEN_SPACE_OCCLUSION
		
	#endif
#endif

#if !defined(_DISABLE_LIGHTMAPS)
	//#pragma multi_compile_fragment _ _MIXED_LIGHTING_SUBTRACTIVE
	#pragma multi_compile _ LIGHTMAP_SHADOW_MIXING
	#pragma multi_compile _ DIRLIGHTMAP_COMBINED
	#pragma multi_compile _ LIGHTMAP_ON
	#pragma multi_compile _ DYNAMICLIGHTMAP_ON
#endif

#pragma multi_compile _ SHADOWS_SHADOWMASK
#pragma multi_compile_fragment _ _LIGHT_COOKIES

#if defined(_SCREEN_SPACE_OCCLUSION_KEYWORD_DECLARED)
#define BRANCH_SCREEN_SPACE_OCCLUSION _SCREEN_SPACE_OCCLUSION
#else
#define BRANCH_SCREEN_SPACE_OCCLUSION 0
#endif