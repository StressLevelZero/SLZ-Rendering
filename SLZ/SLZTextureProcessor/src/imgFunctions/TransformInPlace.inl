#if !defined(PIX_TYPE)

#define PIX_TYPE fix8
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix8v2
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix8v3
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix8v4
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v2
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v3
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE fix16v4
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE

#define PIX_TYPE half
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE half2
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE half3
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE half4
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE

#define PIX_TYPE float
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE float2
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE float3
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE 

#define PIX_TYPE float4
#include "imgFunctions/TransformInPlace.inl"
#undef PIX_TYPE

void TransformInPlace(TxpTextureFormat fmt, void* imageIn, int2 resolution, mat4s transform, vec4s offset)
{
    switch (fmt)
    {
        case FMT_R8:        TransformInPlace_fix8   (imageIn, resolution, transform, offset); break;
        case FMT_RG8:		TransformInPlace_fix8v2 (imageIn, resolution, transform, offset); break;
        case FMT_RGB8:		TransformInPlace_fix8v3 (imageIn, resolution, transform, offset); break;
        case FMT_RGBA8:		TransformInPlace_fix8v4 (imageIn, resolution, transform, offset); break;
        case FMT_R16:		TransformInPlace_fix16  (imageIn, resolution, transform, offset); break;
        case FMT_RG16:		TransformInPlace_fix16v2(imageIn, resolution, transform, offset); break;
        case FMT_RGB16:		TransformInPlace_fix16v3(imageIn, resolution, transform, offset); break;
        case FMT_RGBA16:	TransformInPlace_fix16v4(imageIn, resolution, transform, offset); break;
        case FMT_RHalf:	    TransformInPlace_half   (imageIn, resolution, transform, offset); break;
        case FMT_RGHalf:	TransformInPlace_half2  (imageIn, resolution, transform, offset); break;
        case FMT_RGBHalf:	TransformInPlace_half3  (imageIn, resolution, transform, offset); break;
        case FMT_RGBAHalf:	TransformInPlace_half4  (imageIn, resolution, transform, offset); break;
        case FMT_RFloat:    TransformInPlace_float  (imageIn, resolution, transform, offset); break;
        case FMT_RGFloat:	TransformInPlace_float2 (imageIn, resolution, transform, offset); break;
        case FMT_RGBFloat:	TransformInPlace_float3 (imageIn, resolution, transform, offset); break;
        case FMT_RGBAFloat:	TransformInPlace_float4 (imageIn, resolution, transform, offset); break;
        default: break;
    }
}

#else


#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

void CCAT(TransformInPlace_, PIX_TYPE)(PIX_TYPE* imageIn, int2 resolution, mat4s transform, vec4s offset)
{
    //const int typeSize = sizeof(imageIn[0].raw) / sizeof(imageIn[0].raw[0]);

    long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(imageIn)
    for (long i = 0; i < totalPixels; i++)
    {
        vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
        vec4s rotScaled = glms_mat4_mulv(transform, fPixel);
        vec4s outPixel = glms_vec4_add(rotScaled, offset);
        imageIn[i] = FLOAT4_TO_PIX(imageIn, outPixel);
    }
}

#undef CCAT2 
#undef CCAT

#endif