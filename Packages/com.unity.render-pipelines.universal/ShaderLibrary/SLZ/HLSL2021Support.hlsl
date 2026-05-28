#ifndef SLZ_HLSL2021_SUPPORT
#define SLZ_HLSL2021_SUPPORT

// Currently, the only way to get HLSL 2021 is to use our modified version of the DXC compiler that forces -HV 2021
// Unity runs it's own preprocessor to resolve all symbols before sending it to the compiler, so I have to resort
// to some macro bullshit to construct the preprocessor branches with unity's preprocessor so it can't parse them

#define COMPILER_MACRO(hash, value) hash value

#if defined(UNITY_COMPILER_DXC)
    COMPILER_MACRO(#, if defined(__HLSL_VERSION) && __HLSL_VERSION >= 2021)
    COMPILER_MACRO(#, define HLSL_2021)
    COMPILER_MACRO(#, endif)
    
    COMPILER_MACRO(#, if !defined(HLSL_2021))
    COMPILER_MACRO(#, define select(a, b, c) ((a) ? (b) : (c)))
    COMPILER_MACRO(#, define and(a, b) ((a) && (b)))
    COMPILER_MACRO(#, define or(a, b) ((a) || (b)))
    COMPILER_MACRO(#, endif)
#endif 

// Define HLSL2021's vector logic functions to equivalents for pre-HLSL2021. These were introduced to allow short-circuiting
// ternary operations on non-vector conditions.
#if !defined(UNITY_COMPILER_DXC)
    #define select(a, b, c) ((a) ? (b) : (c))
    #define and(a, b) ((a) && (b))
    #define or(a, b) ((a) || (b))
#endif

#endif // SLZ_HLSL2021_SUPPORT
