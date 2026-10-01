#if !defined(SLZ_HLSL2021)
#define SLZ_HLSL2021

#include "Packages/com.stresslevelzero.urpconfig/include/DXCUpdateState.hlsl"

// Define 'select' function introduced into later versions of DXC
// Ternary operations on vectors are no longer legal, select should be used instead
#define COMPILER_MACRO(hash, value) hash value
#define COMPILER_MACRO3(hash, func, params, value) hash define func##params value

#if defined(UNITY_COMPILER_DXC)
    COMPILER_MACRO(#, if defined(__HLSL_VERSION) && __HLSL_VERSION >= 2021)
    COMPILER_MACRO(#, define HLSL_2021)
    COMPILER_MACRO(#, endif)

    #if defined(SLZ_DXC_HAS_TEMPLATES)
        #define TEMPLATES_SUPPORTED
    #endif

    #if !defined(SLZ_DXC_HAS_SELECT)
    #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/SLZ/HLSL2021SupportTemplates.hlsl"
    #endif

#endif 

// Define HLSL2021's vector logic functions to equivalents for pre-HLSL2021. These were introduced to allow short-circuiting
// ternary operations on non-vector conditions.
#if !defined(UNITY_COMPILER_DXC)
    COMPILER_MACRO(#, define __HLSL_VERSION 2015)
    #define select(a, b, c) ((a) ? (b) : (c))
    #define and(a, b) ((a) && (b))
    #define or(a, b) ((a) || (b))
#endif

#endif // SLZ_HLSL2021