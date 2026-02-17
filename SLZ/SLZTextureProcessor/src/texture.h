#if !defined(SLZ_TEXTURE)
#define SLZ_TEXTURE

#include <stdlib.h>
#include <math.h>
#include <stdalign.h>
#include "cglm/struct.h"
#if defined(_MSC_VER) && !defined(__clang__)
#include <immintrin.h>
#endif

#define STATIC_INLINE inline

#define MAX_TEXTURE_RESOLUTION 16384
#define MAX_TEXTURE_RESOLUTION_LOG2 14
/*
typedef enum TxpTextureFormat
{
    FMT_UNKNOWN = 0,
    FMT_R8, 
    FMT_RG8, 
    FMT_RGB8,
    FMT_RGBA8,
    FMT_R16,
    FMT_RG16,
    FMT_RGB16,
    FMT_RGBA16,
    FMT_RHalf,
    FMT_RGHalf,
    FMT_RGBHalf,
    FMT_RGBAHalf,
    FMT_RFloat,
    FMT_RGFloat,
    FMT_RGBFloat,
    FMT_RGBAFloat
} TxpTextureFormat;

typedef enum TxpTextureChannelFmt
{
    CHANNEL_FMT_UNKNOWN,
    CHANNEL_FMT_FIXED8,
    CHANNEL_FMT_FIXED16,
    CHANNEL_FMT_FLOAT16,
    CHANNEL_FMT_FLOAT32
} TxpTextureChannelFmt;
*/

typedef unsigned char fix8;

typedef struct fix8v2
{
    fix8 r;
    fix8 g;
} fix8v2;

typedef struct fix8v3
{
    fix8 r;
    fix8 g;
    fix8 b;
} fix8v3;

struct fix8v4
{
    fix8 r;
    fix8 g;
    fix8 b;
    fix8 a;
};
typedef struct fix8v4 fix8v4;

typedef signed char sfix8;

typedef struct sfix8v2
{
    sfix8 r;
    sfix8 g;
} sfix8v2;

typedef struct sfix8v3
{
    sfix8 r;
    sfix8 g;
    sfix8 b;
} sfix8v3;

typedef struct sfix8v4
{
    sfix8 r;
    sfix8 g;
    sfix8 b;
    sfix8 a;
} sfix8v4;

typedef unsigned short fix16;

typedef struct fix16v2
{
    fix16 r;
    fix16 g;
} fix16v2;

typedef struct fix16v3
{
    fix16 r;
    fix16 g;
    fix16 b;
} fix16v3;

typedef struct fix16v4
{
    fix16 r;
    fix16 g;
    fix16 b;
    fix16 a;
} fix16v4;

typedef signed short sfix16;

typedef struct sfix16v2
{
    sfix16 r;
    sfix16 g;
} sfix16v2;

typedef struct sfix16v3
{
    sfix16 r;
    sfix16 g;
    sfix16 b;
} sfix16v3;

typedef struct sfix16v4
{
    sfix16 r;
    sfix16 g;
    sfix16 b;
    sfix16 a;
} sfix16v4;
#if defined(__clang__) || defined(__GNUC__)
typedef _Float16 half;
#else
typedef unsigned short half;
#endif

typedef struct half2
{
    half r;
    half g;
} half2;

typedef struct half3
{
    half r;
    half g;
    half b;
} half3;

typedef struct half4
{
    half r;
    half g;
    half b;
    half a;
} half4;

typedef vec2s float2;
typedef vec3s float3;
typedef vec4s float4;

typedef struct MipLevel
{
    int2 resolution;
    void* data;
} MipLevel;

/*
// Keep in sync with the struct of the same name in the C# ScriptedImporter
typedef struct TxpTex2D
{
    TxpTextureFormat format;
    int32_t mipCount;
    int2* resolution;
    void** mips;
    int64_t pad0;
} TxpTex2D;
*/

inline int ChannelFmtToBytes(TxpTextureChannelFmt fmt)
{
    switch (fmt)
    {
    case (CHANNEL_FMT_FIXED8): return 1;
    case (CHANNEL_FMT_FIXED16): return 2;
    case (CHANNEL_FMT_FLOAT16): return 2;
    case (CHANNEL_FMT_FLOAT32): return 4;
    default: return 0;
    }
}

static TxpTextureFormat TextureFormatFromChannelInfo(TxpTextureChannelFmt channelFmt, int channelCount)
{
    switch (channelFmt)
    {
    case (CHANNEL_FMT_FIXED8):
        switch (channelCount)
        {
        case 1: return FMT_R8;
        case 2: return FMT_RG8;
        case 3: return FMT_RGB8;
        case 4: return FMT_RGBA8;
        }
        break;
    case (CHANNEL_FMT_FIXED16):
        switch (channelCount)
        {
        case 1: return FMT_R16;
        case 2: return FMT_RG16;
        case 3: return FMT_RGB16;
        case 4: return FMT_RGBA16;
        }
        break;
    case (CHANNEL_FMT_FLOAT16):
        switch (channelCount)
        {
        case 1: return FMT_RHalf;
        case 2: return FMT_RGHalf;
        case 3: return FMT_RGBHalf;
        case 4: return FMT_RGBAHalf;
        }
        break;
    case (CHANNEL_FMT_FLOAT32):
        switch (channelCount)
        {
        case 1: return FMT_RFloat;
        case 2: return FMT_RGFloat;
        case 3: return FMT_RGBFloat;
        case 4: return FMT_RGBAFloat;
        }
        break;
    default: 
        break;
    }
    DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: Failed to get texture format with %d channels and TxpTextureChannelFmt %d\n", channelCount, channelFmt);

    return FMT_UNKNOWN;
}

// unsigned fix8 conversions -------------------------------------------------

STATIC_INLINE  float Fix8ToFloat(const fix8 value)
{
    return (float)value * (1.0f / 255.0f);
}

STATIC_INLINE  fix8 FloatToFix8(const float value)
{
    return (fix8)(roundf(fmaxf(fminf(1.0f, value), 0.0) * 255.0f));
}

#define PIX_VEC fix8v
#define PIX_VECU Fix8v
#define PIX_TO_FLOAT Fix8ToFloat
#define FLOAT_TO_PIX FloatToFix8

#include "imgFunctions/pixelFloatConversions.inl"

#undef PIX_VEC
#undef PIX_VECU
#undef PIX_TO_FLOAT
#undef FLOAT_TO_PIX

// signed fix8 conversions ---------------------------------------------------

STATIC_INLINE  float SFix8ToFloat(const sfix8 value)
{
    return (float)value * (1.0f / 127.0f);
}

STATIC_INLINE  sfix8 FloatToSFix8(const float value)
{
    return (fix8)(roundf(fmaxf(fminf(1.0f, value), -1.0f) * 127.0f));
}

#define PIX_VEC sfix8v
#define PIX_VECU SFix8v
#define PIX_TO_FLOAT SFix8ToFloat
#define FLOAT_TO_PIX FloatToSFix8

#include "imgFunctions/pixelFloatConversions.inl"

#undef PIX_VEC
#undef PIX_VECU
#undef PIX_TO_FLOAT
#undef FLOAT_TO_PIX

// unsigned fix16 conversions ------------------------------------------------

STATIC_INLINE  float Fix16ToFloat(const fix16 value)
{
    return (float)value * (1.0f / 65535.0f);
}

STATIC_INLINE  fix16 FloatToFix16(const float value)
{
    return (fix16)(roundf(fmaxf(fminf(1.0f, value), 0.0) * 65535.0f));
}

#define PIX_VEC fix16v
#define PIX_VECU Fix16v
#define PIX_TO_FLOAT Fix16ToFloat
#define FLOAT_TO_PIX FloatToFix16

#include "imgFunctions/pixelFloatConversions.inl"

#undef PIX_VEC
#undef PIX_VECU
#undef PIX_TO_FLOAT
#undef FLOAT_TO_PIX

// signed fix16 conversions --------------------------------------------------

STATIC_INLINE  float SFix16ToFloat(const fix16 value)
{
    return (float)value * (1.0f / 32767.0f);
}

STATIC_INLINE  fix16 FloatToSFix16(const float value)
{
    return (fix16)(roundf(fmaxf(fminf(1.0f, value), -1.0) * 32767.0f));
}

#define PIX_VEC sfix16v
#define PIX_VECU SFix16v
#define PIX_TO_FLOAT SFix16ToFloat
#define FLOAT_TO_PIX FloatToSFix16

#include "imgFunctions/pixelFloatConversions.inl"

#undef PIX_VEC
#undef PIX_VECU
#undef PIX_TO_FLOAT
#undef FLOAT_TO_PIX

// half conversions ----------------------------------------------------------

#if !defined(_MSC_VER) || defined(__clang__)
STATIC_INLINE  float HalfToFloat(const half value)
{
    return (float)value;
}

STATIC_INLINE  half FloatToHalf(const float value)
{
    return (half)value;
}

#define PIX_VEC half
#define PIX_VECU Half
#define PIX_TO_FLOAT HalfToFloat
#define FLOAT_TO_PIX FloatToHalf

#include "imgFunctions/pixelFloatConversions.inl"

#undef PIX_VEC
#undef PIX_VECU
#undef PIX_TO_FLOAT
#undef FLOAT_TO_PIX

#else
// MSVC doesn't support _Float16

STATIC_INLINE  vec4s Half4ToFloat4(const half4* value)
{
    __declspec(align(16)) half4 test[2] = { value, value };
    __declspec(align(16)) vec4s output;
    __m128i h = _mm_load_si128(test);
    __m128 f = _mm_cvtph_ps(h);
    _mm_store_ps(output.raw, f);
    return output;
}

STATIC_INLINE  half4 Float4ToHalf4(const vec4s* value)
{
    __declspec(align(16)) half4 output[2];
    __m128 f = _mm_load_ps(value);
    __m128i h = _mm_cvtps_ph(f, 0);
    _mm_storeu_epi16(output, h);
    return output[0];
}

STATIC_INLINE  float HalfToFloat(const half value)
{
    half4 vec = { value, 0u, 0u, 0u };
    vec4s out = Half4ToFloat4(&vec);
    return out.x;
}

STATIC_INLINE  half FloatToHalf(const float value)
{
    vec4s vec = { value, 0, 0, 0 };
    half4 out = Float4ToHalf4(&vec);
    return out.r;
}

STATIC_INLINE  vec2s Half2ToFloat2(const half2* value)
{
    half4 vec = { value->r, value->g, 0u, 0u };
    vec4s out4 = Half4ToFloat4(&vec);
    vec2s out = { out.x, out.y };
    return out;
}

STATIC_INLINE  vec3s Half3ToFloat3(const half3* value)
{
    half4 vec = { value->r, value->g, value->b, 0u };
    vec4s out4 = Half4ToFloat4(&vec);
    vec3s out = { out.x, out.y, out.z };
    return out;
}

STATIC_INLINE  vec4s Half3ToFloat4( half3* value)
{
    half4 vec = { value->r, value->g, value->b, 0u };
    vec4s out4 = Half4ToFloat4(&vec);
    return out4;
}

STATIC_INLINE  vec4s Half2ToFloat4(const half2* value)
{
    half4 vec = { value->r, value->g, 0u, 0u };
    vec4s out4 = Half4ToFloat4(&vec);
    return out4;
}

STATIC_INLINE  half2 Float2ToHalf2(const vec2s* value)
{
    vec4s vec = { value->x, value->y, 0, 0};
    half4 out4 = Float4ToHalf4(&vec);
    half2 out = { out.r, out.g };
    return out;
}

STATIC_INLINE  half3 Float3ToHalf3(const vec3s* value)
{
    vec4s vec = { value->x, value->y, value->z, 0 };
    half4 out4 = Float4ToHalf4(&vec);
    half3 out = { out.r, out.g, out.b };
    return out;
}

STATIC_INLINE  half3 Float4ToHalf3(const vec4s* value)
// STATIC_INLINE  fixed8v3 Float4ToFixed8v2(const vec4s* value)
{
    half4 out4 = Float4ToHalf4(&value);
    half3 out = { out.r, out.g, out.b };
    return out;
}

STATIC_INLINE  half2 Float4ToHalf2(const vec4s* value)
// STATIC_INLINE  fixed8v2 Float4ToFixed8v2(const vec4s* value)
{
    half4 out4 = Float4ToHalf4(&value);
    half2 out = { out.r, out.g };
    return out;
}

#endif



STATIC_INLINE  float4 Float4ToFloat4(const float4 value)
{
    return (float4) { value.x, value.y, value.z, value.w };
}

STATIC_INLINE  float3 Float4ToFloat3(const float4 value)
{
    return (float3) { value.x, value.y, value.z };
}

STATIC_INLINE  float2 Float4ToFloat2(const float4 value)
{
    return (float2) { value.x, value.y };
}

STATIC_INLINE  float Float4ToFloat(const float4 value)
{
    return value.x;
}

STATIC_INLINE  float4 Float3ToFloat4(const float3 value)
{
    return (float4) { value.x, value.y, value.z, 0.0f };
}

STATIC_INLINE  float4 Float2ToFloat4(const float2 value)
{
    return (float4) { value.x, value.y, 0.0f, 0.0f };
}

STATIC_INLINE  float4 FloatToFloat4(const float value)
{
    return (float4) { value, 0.0f, 0.0f, 0.0f };
}

// fix8 to fix16 ----------------------------------------------------------

STATIC_INLINE  fix16 Fix8ToFix16(const fix8 value)
{
    return (fix16)((unsigned int)value * 257u);
}

STATIC_INLINE  fix16v2 Fix8v2ToFix16v2(const fix8v2 value)
{
    fix16v2 output =
    {
        Fix8ToFix16(value.r),
        Fix8ToFix16(value.g)
    };
    return output;
}

STATIC_INLINE  fix16v3 Fix8v3ToFix16v3(const fix8v3 value)
{
    fix16v3 output =
    {
        Fix8ToFix16(value.r),
        Fix8ToFix16(value.g),
        Fix8ToFix16(value.b)
    };
    return output;
}

STATIC_INLINE  fix16v4 Fix8v4ToFix16v4(const fix8v4 value)
{
    fix16v4 output =
    {
        Fix8ToFix16(value.r),
        Fix8ToFix16(value.g),
        Fix8ToFix16(value.b),
        Fix8ToFix16(value.a)
    };
    return output;
}

// fix16 to fix8 ----------------------------------------------------------

STATIC_INLINE  fix8 Fix16ToFix8(const fix16 value)
{
    return (fix8)((unsigned int)value / 257u);
}

STATIC_INLINE  fix8v2 Fix16v2ToFix8v2(const fix16v2 value)
{
    fix8v2 output =
    {
        Fix16ToFix8(value.r),
        Fix16ToFix8(value.g)
    };
    return output;
}

STATIC_INLINE  fix8v3 Fix16v3ToFix8v3(const fix16v3 value)
{
    fix8v3 output =
    {
        Fix16ToFix8(value.r),
        Fix16ToFix8(value.g),
        Fix16ToFix8(value.b)
    };
    return output;
}

STATIC_INLINE  fix8v4 Fix16v4ToFix8v4(const fix16v4 value)
{
    fix8v4 output =
    {
        Fix16ToFix8(value.r),
        Fix16ToFix8(value.g),
        Fix16ToFix8(value.b),
        Fix16ToFix8(value.a)
    };
    return output;
}

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static unsigned int ceilPow2(unsigned int x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

static int floorPow2(int x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x >> 1;
}

// TxpTex2D functions

static size_t TextureFormatSize(enum TxpTextureFormat format)
{
    size_t outp = 0;
    switch (format)
    {
    case FMT_R8: outp = 1; break;
    case FMT_RG8: outp = 2; break;
    case FMT_RGB8: outp = 3; break;
    case FMT_RGBA8: outp = 4; break;
    case FMT_R16: outp = 2; break;
    case FMT_RG16: outp = 4; break;
    case FMT_RGB16: outp = 6; break;
    case FMT_RGBA16: outp = 8; break;
    case FMT_RHalf: outp = 2; break;
    case FMT_RGHalf: outp = 4; break;
    case FMT_RGBHalf: outp = 6; break;
    case FMT_RGBAHalf: outp = 8; break;
    case FMT_RFloat: outp = 4; break;
    case FMT_RGFloat: outp = 8; break;
    case FMT_RGBFloat: outp = 12; break;
    case FMT_RGBAFloat: outp = 16; break;
    default: outp = 0;
    }
    return outp;
}

static TXPErrorCode CreateTexture2DSize(TxpTex2D** outputPtr, int width, int height, size_t formatSize, int maxMips)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;

    if (width <= 0 || height <= 0)
    {
        err = TXP_RETURN_ZERO_SIZE_TEX;
        goto exit;
    }

    int widthPow2 = floor(log2(width));
    int heightPow2 = floor(log2(height));
    int mipCount = imax(widthPow2, heightPow2) + 1;
    if (maxMips > 0)
    {
        mipCount = imin(mipCount, maxMips);
    }
    //printf("Mip Count: %d\n", mipCount);

    // max resolution is 16K (2^14), so we can't have more than 14 mip levels
    size_t alloc_offset[MAX_TEXTURE_RESOLUTION_LOG2];
    unsigned short mipWidths[MAX_TEXTURE_RESOLUTION_LOG2]; 
    unsigned short mipHeights[MAX_TEXTURE_RESOLUTION_LOG2];
    
    // calculate the total memory needed for the texture struct and all mip levels as well as the resolution of each mip and its offset in memory
    size_t alloc_size = sizeof(TxpTex2D);
    alloc_size += sizeof(int2) * mipCount;
    // offset of the beginning of the array of pointers to the memory for each mip level 
    size_t mipPtrBegin = alloc_size;
    alloc_size = ((alloc_size + sizeof(void*) * mipCount + 15) / 16) * 16; // round to nearest 16 bytes for alignment
    for (int mipIdx = 0; mipIdx < mipCount; mipIdx++)
    {
        int mipWidth = max(1, width >> mipIdx);
        int mipHeight = max(1, height >> mipIdx);
        mipWidths[mipIdx] = mipWidth;
        mipHeights[mipIdx] = mipHeight;
        alloc_offset[mipIdx] = alloc_size;
        alloc_size += ((mipWidth * mipHeight * formatSize + 15) / 16) * 16; // round up to nearest 16 bytes for alignment
    }

    *outputPtr = (TxpTex2D*)malloc(alloc_size);
    TxpTex2D* output = *outputPtr;
    if (output == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }

    //output->format = format;
    output->mipCount = mipCount;
    output->resolution = (int2*)((char*)output + sizeof(TxpTex2D)); // resolution array begins after TxpTex2D struct
    output->mips = (void**)((char*)output + mipPtrBegin); // mip pointer array begins after resolution array

    for (int mipIdx = 0; mipIdx < mipCount; mipIdx++)
    {
        output->mips[mipIdx] = (void*)((char*)output + alloc_offset[mipIdx]);
        output->resolution[mipIdx].x = mipWidths[mipIdx];
        output->resolution[mipIdx].y = mipHeights[mipIdx];
        //size_t mipSize = mipWidths[mipIdx] * mipHeights[mipIdx] * formatSize;
        //printf("Mip %d:\n    size:      %llu\n    start:      %llu\n    end:      %llu\n    remaining:      %llu\n",
        //	mipIdx, 
        //	mipSize,
        //	(size_t)output->mips[mipIdx],
        //	(size_t)output->mips[mipIdx] + mipSize,
        //	(size_t)output + alloc_size - ((size_t)output->mips[mipIdx] + mipSize)
        //	);
    }

    exit:
        return err;
}

static TXPErrorCode CreateTexture2D(TxpTex2D** outputPtr, int width, int height, enum TxpTextureFormat format, int maxMips)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    size_t formatSize = TextureFormatSize(format);
    if (formatSize == 0)
    {
        err = TXP_RETURN_INVALID_TEX_FORMAT;
        goto exit;
    }

    err = CreateTexture2DSize(outputPtr, width, height, formatSize, maxMips);
    TXP_HANDLE_ERROR(err, exit)

    (*outputPtr)->format = format;

    exit:
        return err;
}

static void DisposeTexture2D(TxpTex2D* texture)
{
    if (texture != NULL)
    {
        free(texture); //
    }
}

#define TYPE_TO_FMT(VAR) _Generic((VAR),\
    fix8v2*		: FMT_RG8,		\
    fix8v3*		: FMT_RGB8,		\
    fix8v4*		: FMT_RGBA8,	\
    fix16v2*	: FMT_RG16,		\
    fix16v3*	: FMT_RGB16,	\
    fix16v4*	: FMT_RGBA16,	\
    sfix8v2*	: FMT_RG8,		\
    sfix8v3*	: FMT_RGB8,		\
    sfix8v4*	: FMT_RGBA8,	\
    sfix16v2*	: FMT_RG16,		\
    sfix16v3*	: FMT_RGB16,	\
    sfix16v4*	: FMT_RGBA16,	\
    half2*		: FMT_RGHalf,	\
    half3*		: FMT_RGBHalf,	\
    half4*		: FMT_RGBAHalf \
)\


#define PIX_TO_FLOAT4(VAR) _Generic((&(VAR)), \
    fix8v2* : Fix8v2ToFloat4, \
    fix8v3* : Fix8v3ToFloat4, \
    fix8v4* : Fix8v4ToFloat4, \
    fix16v2* : Fix16v2ToFloat4, \
    fix16v3* : Fix16v3ToFloat4, \
    fix16v4* : Fix16v4ToFloat4, \
    sfix8v2* : SFix8v2ToFloat4, \
    sfix8v3* : SFix8v3ToFloat4, \
    sfix8v4* : SFix8v4ToFloat4, \
    sfix16v2* : SFix16v2ToFloat4, \
    sfix16v3* : SFix16v3ToFloat4, \
    sfix16v4* : SFix16v4ToFloat4, \
    half2*   : Half2ToFloat4, \
    half3*   : Half3ToFloat4, \
    half4*   : Half4ToFloat4, \
    float2*  : Float2ToFloat4,\
    float3* : Float3ToFloat4, \
    float4* : Float4ToFloat4, \
    const fix8v2* : Fix8v2ToFloat4, \
    const fix8v3* : Fix8v3ToFloat4, \
    const fix8v4* : Fix8v4ToFloat4, \
    const fix16v2* : Fix16v2ToFloat4, \
    const fix16v3* : Fix16v3ToFloat4, \
    const fix16v4* : Fix16v4ToFloat4, \
    const sfix8v2* : SFix8v2ToFloat4, \
    const sfix8v3* : SFix8v3ToFloat4, \
    const sfix8v4* : SFix8v4ToFloat4, \
    const sfix16v2* : SFix16v2ToFloat4, \
    const sfix16v3* : SFix16v3ToFloat4, \
    const sfix16v4* : SFix16v4ToFloat4, \
    const half2*   : Half2ToFloat4, \
    const half3*   : Half3ToFloat4, \
    const half4*   : Half4ToFloat4, \
    const float2* : Float2ToFloat4, \
    const float3* : Float3ToFloat4, \
    const float4* : Float4ToFloat4  \
    )(VAR)

#define FLOAT4_TO_PIX(OUTP, VAR) _Generic((OUTP), \
    fix8v2* : Float4ToFix8v2, \
    fix8v3* : Float4ToFix8v3, \
    fix8v4* : Float4ToFix8v4, \
    fix16v2* : Float4ToFix16v2, \
    fix16v3* : Float4ToFix16v3, \
    fix16v4* : Float4ToFix16v4, \
    sfix8v2* : Float4ToSFix8v2, \
    sfix8v3* : Float4ToSFix8v3, \
    sfix8v4* : Float4ToSFix8v4, \
    sfix16v2* : Float4ToSFix16v2, \
    sfix16v3* : Float4ToSFix16v3, \
    sfix16v4* : Float4ToSFix16v4, \
    half2*   : Float4ToHalf2, \
    half3*   : Float4ToHalf3, \
    half4*   : Float4ToHalf4, \
    float2*  : Float4ToFloat2,\
    float3* : Float4ToFloat3, \
    float4*  : Float4ToFloat4,\
    const fix8v2* : Float4ToFix8v2, \
    const fix8v3* : Float4ToFix8v3, \
    const fix8v4* : Float4ToFix8v4, \
    const fix16v2* : Float4ToFix16v2, \
    const fix16v3* : Float4ToFix16v3, \
    const fix16v4* : Float4ToFix16v4, \
    const sfix8v2* : Float4ToSFix8v2, \
    const sfix8v3* : Float4ToSFix8v3, \
    const sfix8v4* : Float4ToSFix8v4, \
    const sfix16v2* : Float4ToSFix16v2, \
    const sfix16v3* : Float4ToSFix16v3, \
    const sfix16v4* : Float4ToSFix16v4, \
    const half2*   : Float4ToHalf2, \
    const half3*   : Float4ToHalf3, \
    const half4*   : Float4ToHalf4, \
    const float2* : Float4ToFloat2, \
    const float3* : Float4ToFloat3, \
    const float4* : Float4ToFloat4  \
    )(VAR)

#pragma omp declare simd
STATIC_INLINE  vec4s HemiOctToVec(float x, float y)
{
    vec2s xy = { x + y, x - y };
    float z = 2.0f - (fabsf(xy.x) + fabs(xy.y));
    // Reverse the 45 degree rotation applied to the vector prior to encoding. This is done so that that gradients in hemi-oct coordinates
    // are aligned with the change in UV direction. Normal maps tend to 
    float x2 = 0.70710678118654752 * (xy.x + xy.y);
    float y2 = 0.70710678118654752 * (-xy.x + xy.y);
    vec3s vect = { x2, y2, z };

    vect = glms_vec3_normalize(vect);

    vec4s outp = { vect.x, vect.y, vect.z, 0 };
    return outp;
}

#pragma omp declare simd
STATIC_INLINE  vec2s VecToHemiOct(const float x, const float y, const float z)
{
    // Rotate the vector 45 degrees prior to encoding. The gradient of the x/y normal components in a normal map tends to be aligned with the normal vector itself. This works nicely with BC6/BC7's partitioning schemes.
    // Hemi-oct coordinates are rotated by 45 on Z relative to the 3d vector space so that the top half of the octahedron fills out the 0-1 coordinate space. This causes the gradient to usually be at a 45 degree
    // angle to the u-v directions, which makes BC6/7 encoders choose diagonal partitions on axis-aligned slopes, and this causes obvious sawtooth artifacts. Simple solution is to apply a 45 degree rotation prior to and
    // after encoding so that the gradients in hemi-oct space are aligned.
    float x2 = 0.70710678118654752f * (x - y);
    float y2 = 0.70710678118654752f * (x + y);
    float norm = (fabsf(x2) + fabsf(y2) + fabsf(z));
    vec2s res = { x2 / norm, y2 / norm };
    vec2s hOct = { 0.5f * (res.x + res.y) + 0.5f, 0.5f * (res.x - res.y) + 0.5f};
    return hOct;
}

#endif

