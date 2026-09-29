#ifndef UNIVERSAL_LIGHT_INCLUDED
#define UNIVERSAL_LIGHT_INCLUDED

/// SLZ MODIFIED 2026-09-29 - Expand light color to half4. We use alpha for uv
#if defined(SLZ_LIGHT_ALPHA_AS_UV)
    #define LIGHT_VEC half4
    #define LIGHT_SWIZZLE rgba
    #define LIGHT_COOKIE_SWIZZLE rgbb
#else
    #define LIGHT_VEC half3
    #define LIGHT_SWIZZLE rgb
    #define LIGHT_COOKIE_SWIZZLE rgb
#endif
/// END SLZ MODIFIED 

// Abstraction over Light shading data.
struct Light
{
    half3 direction;
    /// SLZ MODIFIED 2026-09-29 - Expand light color to half4. We use alpha for uv
    /*
    half3   color;
    */
    LIGHT_VEC color;
    /// END SLZ MODIFIED 2026-09-29
    float   distanceAttenuation; // full-float precision required on some platforms
    half    shadowAttenuation;
    uint    layerMask;
};


#endif // UNIVERSAL_LIGHT_INCLUDED
