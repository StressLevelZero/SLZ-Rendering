#ifndef SLZ_HLSL2021_SUPPORT
#define SLZ_HLSL2021_SUPPORT

// Currently, the only way to get HLSL 2021 is to use our modified version of the DXC compiler that forces -HV2021
// Unfortunately, unity's shader preprocessor resolves all preprocessor macros before they get to the compiler,
// so we can't check the predefined macros for hlsl version :(
// unity uses a fork of the compiler off of version 1.7, ours is version 1.8.0
#if defined(UNITY_COMPILER_DXC) && (SLZ_DXC_VERSION >= 0x00408000)
    #define HLSL_2021
#endif 

// Define HLSL2021's vector logic functions to equivalents for non-HLSL2021. These were introduced to allow short-circuiting
// ternary operations on non-vector conditions.
#if !defined(HLSL_2021)
    #define select(a, b, c) ((a) ? (b) : (c))
    #define and(a, b) ((a) && (b))
    #define or(a, b) ((a) || (b))
#endif

#endif // SLZ_HLSL2021_SUPPORT
