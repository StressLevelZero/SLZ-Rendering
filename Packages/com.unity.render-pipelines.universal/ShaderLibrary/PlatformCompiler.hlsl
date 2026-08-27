/** LEGACY SHADERINCLUDE! DO NOT USE IN NEW SHADERS! **/
#ifndef SLZ_PLATFORM_COMPILER
#define SLZ_PLATFORM_COMPILER
#warning USING LEGACY SHADERINCLUDE PlatformCompiler.hlsl! DO NOT USE IN NEW SHADERS!


#include "Packages/com.stresslevelzero.urpconfig/include/DXCUpdateState.hlsl"

#if (defined(SLZ_DXC_UPDATED) || defined(SHADER_API_DESKTOP)) && !defined(SLZ_NATIVE_PLUGIN_DISABLE_DXC)
#define FORCE_NEW_ARTIFACTS 4
#pragma use_dxc vulkan
#endif

#if defined(SLZ_NATIVE_PLUGIN_DISABLE_DXC)
#pragma never_use_dxc
#endif

// Adding missing stuff here


#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ObsoleteFunctions.hlsl"


#endif