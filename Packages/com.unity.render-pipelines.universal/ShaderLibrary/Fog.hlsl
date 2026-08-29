#pragma once

/// SLZ MODIFIED - Get rid of unity's dogshit shaderconfig that can't be configured without manually copying it out of the fucking packagecache
/*
#include "Packages/com.unity.render-pipelines.universal-config/Runtime/ShaderConfig.cs.hlsl"
*/

#if USE_DYNAMIC_BRANCH_FOG_KEYWORD
#pragma dynamic_branch _ FOG_LINEAR FOG_EXP FOG_EXP2
#else
#pragma multi_compile_fog
#endif
