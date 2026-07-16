#ifndef SLZ_CONSTANTS
#define SLZ_CONSTANTS

// 'Safe' epsilon for rsqrt, this is just picked at random as I can't find any answer as to what is actually safe.
// The problem is rsqrt is a fast approximation with no standardization on how it is calculated. Thus just using 
// the sqrt of the normal float min (about 1.085e-19) is not safe. rsqrt seems to error out at anything smaller than
// half min (6.104e-5) on Quest. Nvidia seems to not ever have issues unless the value is literally 0.
// I've arbitrarily set the safe min to 1e-11 on PC just in case other vendors have issues
#if defined(SHADER_API_MOBILE)
    #define SAFE_FLT_RSQRT_MIN HALF_MIN
#else
    #define SAFE_FLT_RSQRT_MIN 1.0e-11
#endif

#define SLZ_PI_half     half(3.141592653589793238)
#define SLZ_INV_PI_half half(0.318309886183790672)

#define SLZ_SQRT_PI      1.77245385090551602729816
#define SLZ_SQRT_PI_half half(SLZ_SQRT_PI)

// Scale factors for unity's lambert diffuse irradiance SH terms to get the radiance terms
// https://gist.github.com/pema99/f735ca33d1299abe0e143ee94fc61e73

// 2 * sqrt(pi)
#define SH_L0_IRR_TO_RAD 3.54490770181103205
// sqrt(3 * pi)
#define SH_L1_IRR_TO_RAD 3.06998012383946547

#endif