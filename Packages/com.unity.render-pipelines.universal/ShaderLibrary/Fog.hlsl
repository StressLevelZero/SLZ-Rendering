#pragma once

#include "Packages/com.unity.render-pipelines.universal-config/Runtime/ShaderConfig.cs.hlsl"

/* SLZ MODIFIED - Always use dynamic branch fog
#if USE_DYNAMIC_BRANCH_FOG_KEYWORD
*/
#if 1
#pragma dynamic_branch _ FOG_LINEAR FOG_EXP FOG_EXP2
#else
#pragma multi_compile_fog
#endif
