#if !defined(PIX_TYPE_IN)

#define PIX_TYPE_IN fix8
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix8v2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix8v3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix8v4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN float
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_IN


#elif !defined(PIX_TYPE_OUT)

#define PIX_TYPE_OUT fix8
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT

#define PIX_TYPE_OUT fix8v2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix8v3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix8v4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix16
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix16v2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix16v3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix16v4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT

#define PIX_TYPE_OUT half
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT half2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT half3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT half4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT

#define PIX_TYPE_OUT float
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT float2
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT float3
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT float4
#include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT

#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)

#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

void CCAT(ConvertImage_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, TxpTextureFormat outFormat, void* imageOut, int2 resolution)
{
    switch (outFormat)
    {
        case FMT_R8		    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix8   )(imageIn, imageOut, resolution); break;
        case FMT_RG8	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix8v2 )(imageIn, imageOut, resolution); break;
        case FMT_RGB8	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix8v3 )(imageIn, imageOut, resolution); break;
        case FMT_RGBA8	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix8v4 )(imageIn, imageOut, resolution); break;
        case FMT_R16	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix16  )(imageIn, imageOut, resolution); break;
        case FMT_RG16	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix16v2)(imageIn, imageOut, resolution); break;
        case FMT_RGB16	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix16v3)(imageIn, imageOut, resolution); break;
        case FMT_RGBA16	    : CCATN(ConvertImage_ , PIX_TYPE_IN, fix16v4)(imageIn, imageOut, resolution); break;
        case FMT_RHalf	    : CCATN(ConvertImage_ , PIX_TYPE_IN, half   )(imageIn, imageOut, resolution); break;
        case FMT_RGHalf	    : CCATN(ConvertImage_ , PIX_TYPE_IN, half2  )(imageIn, imageOut, resolution); break;
        case FMT_RGBHalf    : CCATN(ConvertImage_ , PIX_TYPE_IN, half3  )(imageIn, imageOut, resolution); break;
        case FMT_RGBAHalf   : CCATN(ConvertImage_ , PIX_TYPE_IN, half4  )(imageIn, imageOut, resolution); break;
        case FMT_RFloat	    : CCATN(ConvertImage_ , PIX_TYPE_IN, float  )(imageIn, imageOut, resolution); break;
        case FMT_RGFloat    : CCATN(ConvertImage_ , PIX_TYPE_IN, float2 )(imageIn, imageOut, resolution); break;
        case FMT_RGBFloat   : CCATN(ConvertImage_ , PIX_TYPE_IN, float3 )(imageIn, imageOut, resolution); break;
        case FMT_RGBAFloat  : CCATN(ConvertImage_ , PIX_TYPE_IN, float4 )(imageIn, imageOut, resolution); break;
        default: break;
    }
}

void CCAT(ConvertSwizzleImage_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, TxpTextureFormat outFormat, void* imageOut, int2 resolution, mat4s swizzleMat, vec4s swizzleAdd)
{
    switch (outFormat)
    {
        case FMT_R8		    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix8   )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RG8	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix8v2 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGB8	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix8v3 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBA8	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix8v4 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_R16	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix16  )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RG16	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix16v2)(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGB16	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix16v3)(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBA16	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, fix16v4)(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RHalf	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, half   )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGHalf	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, half2  )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBHalf    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, half3  )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBAHalf   : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, half4  )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RFloat	    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, float  )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGFloat    : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, float2 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBFloat   : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, float3 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        case FMT_RGBAFloat  : CCATN(ConvertSwizzleImage_ , PIX_TYPE_IN, float4 )(imageIn, imageOut, resolution, swizzleMat, swizzleAdd); break;
        default: break;
    }
}

#undef CCAT2 
#undef CCAT

#undef CCATN2 
#undef CCATN

#else

#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)


#ifndef PIX_TYPE_IN
#error Template type PIX_TYPE_IN not defined
#endif

#ifndef PIX_TYPE_OUT
#error Template type PIX_TYPE_OUT not defined
#endif



void CCATN(ConvertImage_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, PIX_TYPE_OUT* imageOut, int2 resolution)
{
#ifdef DEBUG_PRINT
    printf("Running ConvertImage\n");
#endif
    int isSameType = _Generic(imageIn,
        PIX_TYPE_OUT* : 1,
        default : 0
        );
    if (isSameType)
    {
        memcpy(imageOut, imageIn, sizeof(imageIn[0]) * resolution.x * resolution.y);
    }
    else
    {
        long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(totalPixels, imageIn, imageOut)
        for (long i = 0; i < totalPixels; i++)
        {
            vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
            imageOut[i] = FLOAT4_TO_PIX(imageOut, fPixel);
        }
    }
}

void CCATN(ConvertSwizzleImage_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, PIX_TYPE_OUT* imageOut, int2 resolution, mat4s swizzleMat, vec4s swizzleAdd)
{
#if 1 //def DEBUG_PRINT
    printf("Running ConvertImage\n");
#endif
    bool isSameType = _Generic(imageIn,
        PIX_TYPE_OUT * : true,
        default: false
        );
    bool swizzleIsIdentity =
        swizzleMat.m00 == 1.0f && swizzleMat.m01 == 0.0f && swizzleMat.m02 == 0.0f && swizzleMat.m03 == 0.0f &&
        swizzleMat.m10 == 0.0f && swizzleMat.m11 == 1.0f && swizzleMat.m12 == 0.0f && swizzleMat.m13 == 0.0f &&
        swizzleMat.m20 == 0.0f && swizzleMat.m21 == 0.0f && swizzleMat.m22 == 1.0f && swizzleMat.m23 == 0.0f &&
        swizzleMat.m30 == 0.0f && swizzleMat.m31 == 0.0f && swizzleMat.m32 == 0.0f && swizzleMat.m33 == 1.0f;

    bool noAdd = swizzleAdd.x == 0.0f && swizzleAdd.y == 0.0f && swizzleAdd.z == 0.0f && swizzleAdd.w == 0.0f;

    if (isSameType && swizzleIsIdentity && noAdd)
    {
        memcpy(imageOut, imageIn, sizeof(imageIn[0]) * resolution.x * resolution.y);
    }
    else if (swizzleIsIdentity && noAdd)
    {
        long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(totalPixels, imageIn, imageOut)
        for (long i = 0; i < totalPixels; i++)
        {
            vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
            imageOut[i] = FLOAT4_TO_PIX(imageOut, fPixel);
        }
    }
    else
    {
        long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(totalPixels, imageIn, imageOut)
        for (long i = 0; i < totalPixels; i++)
        {
            vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
            vec4s outPixel = glms_vec4_add(glms_mat4_mulv(swizzleMat, fPixel), swizzleAdd);
            imageOut[i] = FLOAT4_TO_PIX(imageOut, outPixel);
        }
    }
}


#undef CCATN2 
#undef CCATN

#endif
