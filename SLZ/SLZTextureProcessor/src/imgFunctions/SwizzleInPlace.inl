#if !defined(PIX_TYPE)

#define PIX_TYPE fix8v2
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix8v3
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix8v4
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v2
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v3
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v4
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE

#define PIX_TYPE half2
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE half3
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE half4
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE

#define PIX_TYPE float2
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE float3
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE float4
#include "imgFunctions/SwizzleInPlace.inl"
#undef PIX_TYPE

void SwizzleInPlace(TxpTextureFormat fmt, void* imageIn, int2 resolution, ivec4s swizzle)
{
    switch (fmt)
    {						
        //case FMT_R8:        SwizzleInPlace_fix8   (imageIn, resolution, swizzle); break;
        case FMT_RG8:		SwizzleInPlace_fix8v2 (imageIn, resolution, swizzle); break;
        case FMT_RGB8:		SwizzleInPlace_fix8v3 (imageIn, resolution, swizzle); break;
        case FMT_RGBA8:		SwizzleInPlace_fix8v4 (imageIn, resolution, swizzle); break;
        //case FMT_R16:		  SwizzleInPlace_fix16  (imageIn, resolution, swizzle); break;
        case FMT_RG16:		SwizzleInPlace_fix16v2(imageIn, resolution, swizzle); break;
        case FMT_RGB16:		SwizzleInPlace_fix16v3(imageIn, resolution, swizzle); break;
        case FMT_RGBA16:	SwizzleInPlace_fix16v4(imageIn, resolution, swizzle); break;
        //case FMT_RHalf:	  SwizzleInPlace_half   (imageIn, resolution, swizzle); break;
        case FMT_RGHalf:	SwizzleInPlace_half2  (imageIn, resolution, swizzle); break;
        case FMT_RGBHalf:	SwizzleInPlace_half3  (imageIn, resolution, swizzle); break;
        case FMT_RGBAHalf:	SwizzleInPlace_half4  (imageIn, resolution, swizzle); break;
        //case FMT_RFloat:	  SwizzleInPlace_float  (imageIn, resolution, swizzle); break;
        case FMT_RGFloat:	SwizzleInPlace_float2 (imageIn, resolution, swizzle); break;
        case FMT_RGBFloat:	SwizzleInPlace_float3 (imageIn, resolution, swizzle); break;
        case FMT_RGBAFloat:	SwizzleInPlace_float4 (imageIn, resolution, swizzle); break;
        default: break;     
    }
}

#else


#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

void CCAT(SwizzleInPlace_, PIX_TYPE)(PIX_TYPE* imageIn, int2 resolution, ivec4s swizzle)
{
    const int typeSize = sizeof(imageIn[0].raw) / sizeof(imageIn[0].raw[0]);
    DebugLog(UNITY_LOG_LEVEL_LOG, "Swizzle type size %d, pixel size: %d, element size %d\n", typeSize, sizeof(imageIn[0].raw), sizeof(imageIn[0].raw[0]));
    bool validIdx[4];
    validIdx[0] = swizzle.x >= 0 && swizzle.x < typeSize;
    validIdx[1] = swizzle.y >= 0 && swizzle.y < typeSize;
    validIdx[2] = swizzle.z >= 0 && swizzle.z < typeSize;
    validIdx[3] = swizzle.w >= 0 && swizzle.w < typeSize;

    long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(imageIn) firstprivate(validIdx, swizzle)
    for (long i = 0; i < totalPixels; i++)
    {
        PIX_TYPE newPixel;
        PIX_TYPE oldPixel = imageIn[i];
        for (int cIdx = 0; cIdx < typeSize; cIdx++)
        {
            if (validIdx[cIdx]) newPixel.raw[cIdx] = oldPixel.raw[swizzle.raw[cIdx]];
        }
        imageIn[i] = newPixel;
    }
}

#undef CCAT2 
#undef CCAT

#endif