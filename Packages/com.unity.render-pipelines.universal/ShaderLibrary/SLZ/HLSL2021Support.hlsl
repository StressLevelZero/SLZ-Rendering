#ifndef SLZ_HLSL2021_SUPPORT
#define SLZ_HLSL2021_SUPPORT

#if !defined(UNITY_COMPILER_DXC) || (SLZ_DXC_VERSION_MINOR < 8 && SLZ_DXC_VERSION_MAJOR <= 1)
    #define select(a, b, c) ((a) ? (b) : (c))
#endif

#endif // SLZ_HLSL2021_SUPPORT
