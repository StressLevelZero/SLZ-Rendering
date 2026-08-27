#include "Platform.h"
#include "omp.h"
#include "main_openmp.h"

#include <threads.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "texture.h"
#include "cglm/struct.h"
#include "cglm/cglm.h"

#define THREAD_COUNT 12
enum imageScaling
{
    IMG_SCALE_EQUAL = 0,
    IMG_SCALE_X = 1,
    IMG_SCALE_Y = 2,
};

inline float MitchellNetravaliWeight(float dist);
TXPErrorCode YFlipImage(void* image, size_t rowSize, int rowCount);

#include "imgFunctions/ConvertImage.inl"

void ConvertImage(TxpTextureFormat inFormat, void* imageIn, TxpTextureFormat outFormat, void* imageOut, int2 resolution)
{
    switch (inFormat)
    {
    case FMT_R8:        ConvertImage_fix8   (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RG8:       ConvertImage_fix8v2 (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGB8:      ConvertImage_fix8v3 (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBA8:     ConvertImage_fix8v4 (imageIn, outFormat, imageOut, resolution); break;
    case FMT_R16:       ConvertImage_fix16  (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RG16:      ConvertImage_fix16v2(imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGB16:     ConvertImage_fix16v3(imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBA16:    ConvertImage_fix16v4(imageIn, outFormat, imageOut, resolution); break;
    case FMT_RHalf:     ConvertImage_half   (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGHalf:    ConvertImage_half2  (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBHalf:   ConvertImage_half3  (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBAHalf:  ConvertImage_half4  (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RFloat:    ConvertImage_float  (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGFloat:   ConvertImage_float2 (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBFloat:  ConvertImage_float3 (imageIn, outFormat, imageOut, resolution); break;
    case FMT_RGBAFloat: ConvertImage_float4 (imageIn, outFormat, imageOut, resolution); break;
    }
}

TXPErrorCode ConvertSwizzleImage(TxpTextureFormat inFormat, void* imageIn, TxpTextureFormat outFormat, void* imageOut, int2 resolution, mat4s swizzleMat, vec4s swizzleAdd)
{
    DebugLog(UNITY_LOG_LEVEL_ERROR, "ConvertSwizzleImage Executing");
    TXPErrorCode result = TXP_RETURN_SUCCESS;
    switch (inFormat)
    {
    case FMT_UNKNOWN:   DebugLog(UNITY_LOG_LEVEL_ERROR, "Cannot convert, format unknown");
    case FMT_R8:        ConvertSwizzleImage_fix8   (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RG8:       ConvertSwizzleImage_fix8v2 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGB8:      ConvertSwizzleImage_fix8v3 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBA8:     ConvertSwizzleImage_fix8v4 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_R16:       ConvertSwizzleImage_fix16  (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RG16:      ConvertSwizzleImage_fix16v2(imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGB16:     ConvertSwizzleImage_fix16v3(imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBA16:    ConvertSwizzleImage_fix16v4(imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RHalf:     ConvertSwizzleImage_half   (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGHalf:    ConvertSwizzleImage_half2  (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBHalf:   ConvertSwizzleImage_half3  (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBAHalf:  ConvertSwizzleImage_half4  (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RFloat:    ConvertSwizzleImage_float  (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGFloat:   ConvertSwizzleImage_float2 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBFloat:  ConvertSwizzleImage_float3 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    case FMT_RGBAFloat: ConvertSwizzleImage_float4 (imageIn, outFormat, imageOut, resolution, swizzleMat, swizzleAdd); break;
    default: result = TXP_RETURN_INVALID_TEX_FORMAT;
    }
    return result;
}

#include "imgFunctions/SwizzleInPlace.inl"

#include "imgFunctions/TransformInPlace.inl"

#include "imgFunctions/scaleMitchellFunc.inl" 

TXPErrorCode ScaleImageMitchell(TxpTextureFormat inFormat, void* imageIn, const int2 resolutionIn, TxpTextureFormat outFormat, void* imageOut, const int2 resolutionOut)
{
    switch (inFormat)
    {
    case FMT_R8:        return ScaleImageMitchell_fix8      (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RG8:       return ScaleImageMitchell_fix8v2    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGB8:      return ScaleImageMitchell_fix8v3    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBA8:     return ScaleImageMitchell_fix8v4    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_R16:       return ScaleImageMitchell_fix16     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RG16:      return ScaleImageMitchell_fix16v2   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGB16:     return ScaleImageMitchell_fix16v3   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBA16:    return ScaleImageMitchell_fix16v4   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RHalf:     return ScaleImageMitchell_half      (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGHalf:    return ScaleImageMitchell_half2     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBHalf:   return ScaleImageMitchell_half3     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBAHalf:  return ScaleImageMitchell_half4     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RFloat:    return ScaleImageMitchell_float     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGFloat:   return ScaleImageMitchell_float2    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBFloat:  return ScaleImageMitchell_float3    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBAFloat: return ScaleImageMitchell_float4    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    default: return TXP_RETURN_INVALID_TEX_FORMAT;
    }
}

void ScaleImageBox(TxpTextureFormat inFormat, void* imageIn, const int2 resolutionIn, TxpTextureFormat outFormat, void* imageOut, const int2 resolutionOut)
{
    switch (inFormat)
    {
    case FMT_R8:        ScaleImageBox_fix8      (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RG8:       ScaleImageBox_fix8v2    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGB8:      ScaleImageBox_fix8v3    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBA8:     ScaleImageBox_fix8v4    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_R16:       ScaleImageBox_fix16     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RG16:      ScaleImageBox_fix16v2   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGB16:     ScaleImageBox_fix16v3   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBA16:    ScaleImageBox_fix16v4   (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RHalf:     ScaleImageBox_half      (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGHalf:    ScaleImageBox_half2     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBHalf:   ScaleImageBox_half3     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBAHalf:  ScaleImageBox_half4     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RFloat:    ScaleImageBox_float     (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGFloat:   ScaleImageBox_float2    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBFloat:  ScaleImageBox_float3    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    case FMT_RGBAFloat: ScaleImageBox_float4    (imageIn, resolutionIn, outFormat, imageOut, resolutionOut); break;
    default: return;
    }
}

#include "imgFunctions/CombineAoSmNrm.inl"


//#define DEBUG_PRINT
#define PIX_TYPE_IN vec3s
#define PIX_TYPE_OUT fix16v3
// #include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT
#undef PIX_TYPE_IN


#define PIX_TYPE_IN vec4s
#define PIX_TYPE_OUT fix16v4
// #include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT
#undef PIX_TYPE_IN

//void ConvertImage_vec4s_fix16v4(vec4s* imageIn, fix16v4* imageOut, int2 resolution)
//{
//    ConvertImage_float4_fix16v4(imageIn, imageOut, resolution);
//}

#define PIX_TYPE_IN half3
#define PIX_TYPE_OUT fix16v3
// #include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half4
#define PIX_TYPE_OUT fix16v4
// #include "imgFunctions/ConvertImage.inl"
#undef PIX_TYPE_OUT
#undef PIX_TYPE_IN

//#undef DEBUG_PRINT


//


#define PIX_TYPE_IN fix8v2
#define PIX_TYPE_OUT fix8v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v3
#define PIX_TYPE_OUT fix8v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v4
#define PIX_TYPE_OUT fix8v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl"
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v2
#define PIX_TYPE_OUT fix8v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v3
#define PIX_TYPE_OUT fix8v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v4
#define PIX_TYPE_OUT fix8v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v4
#define PIX_TYPE_OUT fix8v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v2
#define PIX_TYPE_OUT fix8v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix8v3
#define PIX_TYPE_OUT fix8v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v2
#define PIX_TYPE_OUT fix16v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v3
#define PIX_TYPE_OUT fix16v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v4
#define PIX_TYPE_OUT fix16v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v2
#define PIX_TYPE_OUT fix16v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v3
#define PIX_TYPE_OUT fix16v4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v4
#define PIX_TYPE_OUT fix16v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v4
#define PIX_TYPE_OUT fix16v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v2
#define PIX_TYPE_OUT fix16v3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN fix16v3
#define PIX_TYPE_OUT fix16v2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half2
#define PIX_TYPE_OUT half2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half3
#define PIX_TYPE_OUT half3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half4
#define PIX_TYPE_OUT half4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half2
#define PIX_TYPE_OUT half4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half3
#define PIX_TYPE_OUT half4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
////#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half4
#define PIX_TYPE_OUT half3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half4
#define PIX_TYPE_OUT half2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half2
#define PIX_TYPE_OUT half3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN half3
#define PIX_TYPE_OUT half2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

// Float scale/convert/process -----------------------------------------------

#define PIX_TYPE_IN float2
#define PIX_TYPE_OUT float2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float3
#define PIX_TYPE_OUT float3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float4
#define PIX_TYPE_OUT float4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing.inl"
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float2
#define PIX_TYPE_OUT float4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float3
#define PIX_TYPE_OUT float4
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float4
#define PIX_TYPE_OUT float3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float4
#define PIX_TYPE_OUT float2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float2
#define PIX_TYPE_OUT float3
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

#define PIX_TYPE_IN float3
#define PIX_TYPE_OUT float2
// #include "imgFunctions/ConvertImage.inl"
//#include "imgFunctions/scaleMitchellFunc.inl" 
//#include "imgFunctions/scaleBoxFunc.inl" 
#include "imgFunctions/NormalProcessing2.inl"
#undef PIX_TYPE_IN
#undef PIX_TYPE_OUT

// ---------------------------------------------------

#define PIX_TYPE_1_ENUM FMT_R8
#define PIX_TYPE_2 fix8v2
#define PIX_TYPE_3 fix8v3
#define PIX_TYPE_4 fix8v4

#define PIX_TYPE_IN fix8v2
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix8v3
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix8v4
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_2
#undef PIX_TYPE_3
#undef PIX_TYPE_4

#define PIX_TYPE_1_ENUM FMT_R16
#define PIX_TYPE_2 fix16v2
#define PIX_TYPE_3 fix16v3
#define PIX_TYPE_4 fix16v4

#define PIX_TYPE_IN fix16v2
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix16v3
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix16v4
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_2
#undef PIX_TYPE_3
#undef PIX_TYPE_4

#define PIX_TYPE_1_ENUM FMT_RHalf
#define PIX_TYPE_2 half2
#define PIX_TYPE_3 half3
#define PIX_TYPE_4 half4

#define PIX_TYPE_IN half2
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half3
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half4
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_2
#undef PIX_TYPE_3
#undef PIX_TYPE_4

#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_2
#undef PIX_TYPE_3
#undef PIX_TYPE_4

#define PIX_TYPE_1_ENUM FMT_RFloat
#define PIX_TYPE_2 float2
#define PIX_TYPE_3 float3
#define PIX_TYPE_4 float4

#define PIX_TYPE_IN float2
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN float3
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN float4
#include "imgFunctions/NormalProcessing3.inl"
#undef PIX_TYPE_IN

#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_2
#undef PIX_TYPE_3
#undef PIX_TYPE_4






TXPErrorCode ProcessNormalMap(TxpTextureFormat formatIn, TxpTextureFormat formatOut, void* image, int2 imageRes, TxpTex2D* output,
    ivec4s swizzleMaskIn, ivec4s swizzleMaskOut,
    int yFlip, int detailMap, int hemiOct, float geoRoughnessStr, int genMips)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    switch (formatIn)
    {
    case (FMT_RG8):      err = ProcessNormalMap_fix8v2 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    case (FMT_RGB8):     err = ProcessNormalMap_fix8v3 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    case (FMT_RGBA8):    err = ProcessNormalMap_fix8v4 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    case (FMT_RG16):     err = ProcessNormalMap_fix16v2(formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);  break;
    case (FMT_RGB16):    err = ProcessNormalMap_fix16v3(formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);  break;
    case (FMT_RGBA16):   err = ProcessNormalMap_fix16v4(formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);  break;
    case (FMT_RGHalf):   err = ProcessNormalMap_half2  (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);    break;
    case (FMT_RGBHalf):  err = ProcessNormalMap_half3  (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);    break;
    case (FMT_RGBAHalf): err = ProcessNormalMap_half4  (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);    break;
    case (FMT_RGFloat):  err = ProcessNormalMap_float2 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    case (FMT_RGBFloat): err = ProcessNormalMap_float3 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    case (FMT_RGBAFloat):err = ProcessNormalMap_float4 (formatOut, image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips);   break;
    default: DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: Can't processes image with TxpTextureFormat %d", formatIn); err = TXP_RETURN_INVALID_TEX_FORMAT; break;
    }
    return err;
}


// Heterogeneous type scaling used for generating thumbnails. Thumbnail is always RGBA. Unity doesn't support RGB textures
#define PIX_TYPE_OUT fix8v4

#define PIX_TYPE_IN float2
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN float4
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half2
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half4
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix16v2
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix16v4
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix8v2
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN fix8v4
#include "imgFunctions/GenerateThumbnail.inl"
#undef PIX_TYPE_IN


#undef PIX_TYPE_OUT

TXPErrorCode CreateThumbnail(TxpTextureFormat formatIn, void* imageIn, int2 resIn, void* imageOut, int2 resOut, ivec4s swizzle, int isNormal, int hemiOct)
{
    TXPErrorCode err = TXP_RETURN_GENERAL_FAILURE;
    switch (formatIn)
    {
    case (FMT_RG8):         err = GenerateThumbnail_fix8v2_fix8v4   (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGBA8):       err = GenerateThumbnail_fix8v4_fix8v4   (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RG16):        err = GenerateThumbnail_fix16v2_fix8v4  (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGBA16):      err = GenerateThumbnail_fix16v4_fix8v4  (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGHalf):      err = GenerateThumbnail_half2_fix8v4    (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGBAHalf):    err = GenerateThumbnail_half4_fix8v4    (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGFloat):     err = GenerateThumbnail_float2_fix8v4   (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    case (FMT_RGBAFloat):   err = GenerateThumbnail_float4_fix8v4   (imageIn, resIn, imageOut, resOut, swizzle, isNormal, hemiOct); break;
    default: err = TXP_RETURN_INVALID_TEX_FORMAT; break;
    }
    return err;
}

/** @brief Cacluates a area-aware box filter weight for downscaling an image. 
 *  Same as a normal box filter, except the weight of the pixel is not binary
 *  and instead interpolates towards 0 as the area of the pixel covered by 
 *  the box goes to 0. Not good for upsampling.
 * 
 *  @param outPixelRadius  One half the width of the pixel in the downscaled
 *		image in the coordinate space of the original image
 *  @param outPixelCoord   Horizontal or vertical coordinate of the output 
 *		pixel in the downscaled image, in the coordinate space of the orignal
 *      image
 *  @param inPixelCoord    Coordinate of the pixel in the original image that
 *      will contribute to the downscaled image
 */
inline float AreaBoxWeight(float outPixelRadius, float outPixelCoord, float inPixelCoord)
{
    float distance = fabsf(inPixelCoord - outPixelCoord);
    return distance > (outPixelRadius - 0.5) ? (outPixelRadius + 0.5) - distance : 1;
}

/** @brief Mitchell-Netravali
 * 
 * 
 */
#pragma omp declare simd
inline float MitchellNetravaliWeight(float dist)
{
    const float B = 0.333333f;
    const float C = 0.333333f;

    // cubic coefficients if dist < 1.0
    const float c3 = (12.0f - 9.0f * B - 6.0 * C);
    const float c2 = (-18.0f + 12.0f * B + 6.0 * C);
    const float c1 = 0.0f;
    const float c0 = (6.0f - 2.0f * B);

    // cubic coefficients if 1.0 <= dist < 2.0 
    const float d3 = (-1.0f * B - 6.0f * C);
    const float d2 = (6.0f * B + 30 * C);
    const float d1 = (-12.0f * B - 48.0f * C);
    const float d0 = (8.0f * B + 24.0f * C);

    float f0, f1, f2, f3;
    bool close1 = dist < 1.0f;
    bool close2 = dist < 2.0f;
    f0 = close1 ? c0 : close2 ? d0 : 0.0f;
    f1 = close1 ? c1 : close2 ? d1 : 0.0f;
    f2 = close1 ? c2 : close2 ? d2 : 0.0f;
    f3 = close1 ? c3 : close2 ? d3 : 0.0f;
    
    return  0.6666667f * (f3 * (dist * dist * dist) + f2 * (dist * dist) + f1 * (dist) + f0);
}



void TestScaleImage(fix8v4* imageIn, const int2 resolutionIn, fix8v4* imageOut, const int2 resolutionOut)
{
    ScaleImageMitchell_fix8v4_fix8v4(imageIn, resolutionIn, imageOut, resolutionOut);
    
}



static inline void Pow2Mip_Box_Normal_Var(fix16v4* imageIn, const int2 resIn, fix16v4* imageOut, const int2 resOut, const ivec4s swizzleMask)
{
    #pragma omp for
    for (int rowIdx = 0; rowIdx < resOut.y; rowIdx++)
    {
        int rowIn1Ptr = resIn.x * (2 * rowIdx);
        int rowIn2Ptr = rowIn1Ptr < (resIn.y - 1) ? rowIn1Ptr + resIn.x : rowIn1Ptr;
        int rowOutPtr = resOut.x * rowIdx;

        for (int u = 0; u < resOut.x; u++)
        {
            int xOffset = 2 * u;
            int xOffset2 = xOffset < (resIn.x - 1) ? xOffset + 1 : xOffset;
            fix16v4 pixels[4] =
            {
                imageIn[rowIn1Ptr + xOffset],
                imageIn[rowIn1Ptr + xOffset2],
                imageIn[rowIn2Ptr + xOffset],
                imageIn[rowIn2Ptr + xOffset2]
            };


            vec4s fpixels[4];
#pragma omp simd
            for (int i = 0; i < 4; i++)
            {
                fpixels[i] = PIX_TO_FLOAT4(pixels[i]);
            }

            vec4s normals[4];
            for (int i = 0; i < 4; i++)
            {
                float x = fpixels[i].raw[swizzleMask.x];
                float y = fpixels[i].raw[swizzleMask.y];
                x = 2.0f * x - 1.0f;
                y = 2.0f * y - 1.0f;
                normals[i] = (vec4s){ x, y, sqrtf(fmaxf(1.0f - x * x - y * y, 0.0f)), 0 }; //HemiOctToVec(fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]);
            }

            vec4s avgVec = { 0.25f, 0.25f, 0.25f, 0.25f };
            vec4 avgVecOut;
            glm_mat4_mulv((vec4*)normals, avgVec.raw, avgVecOut);
            vec3s avgNormal = glms_vec3_make(avgVecOut);
            avgNormal = glms_vec3_normalize(avgNormal);

            //vec2s avgHOct = VecToHemiOct(avgNormal.x, avgNormal.y);

            mat4s pixelMat = glms_mat4_transpose(glms_mat4_make((float*)fpixels));
            float avgXSqr = glms_vec4_dot(pixelMat.col[swizzleMask.z], avgVec);
            float avgYSqr = glms_vec4_dot(pixelMat.col[swizzleMask.w], avgVec);
            vec4s outp;
            outp.raw[swizzleMask.x] = 0.5 * avgNormal.x + 0.5;
            outp.raw[swizzleMask.y] = 0.5 * avgNormal.y + 0.5;
            outp.raw[swizzleMask.z] = avgXSqr;
            outp.raw[swizzleMask.w] = avgYSqr;

            imageOut[rowOutPtr + u] = FLOAT4_TO_PIX(imageOut, outp);
        }
    }
}

static inline void Normalize_Var(fix16v4* imageIn, const int2 resIn, const ivec4s swizzleMask, const bool variance)
{
  
    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for
    for (long u = 0; u < pixelCount; u += 4)
    {
        fix16v4 pixels[4];
        int blockCount = min(pixelCount - u, 4);
        for (int i = 0; i < blockCount; i++)
        {
            pixels[i] = imageIn[u + i];
        }
        
        vec4s fpixels[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            fpixels[i] = PIX_TO_FLOAT4(pixels[i]);
        }
        vec4s normals[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            float x = 2.0 * fpixels[i].r - 1.0;
            float y = 2.0 * fpixels[i].g - 1.0;
            normals[i] = (vec4s){ x, y, sqrtf(fmaxf(1.0f - x * x - y * y, 0.0f)), 0 }; //HemiOctToVec(fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]);
            glm_vec3_normalize(normals[i].raw);
            normals[i].x = 0.5 * normals[i].x + 0.5;
            normals[i].y = 0.5 * normals[i].y + 0.5;
            //normals[i].z = 0.5 * normals[i].z + 0.5;
        }

        vec4s outp[4];
        if (variance)
        {
            #pragma omp simd
            for (int i = 0; i < 4; i++)
            {
                outp[i].raw[swizzleMask.x] = normals[i].x;
                outp[i].raw[swizzleMask.y] = normals[i].y;
                outp[i].raw[swizzleMask.z] = normals[i].x * normals[i].x;
                outp[i].raw[swizzleMask.w] = normals[i].y * normals[i].y;
            }
        }
        else
        {
            #pragma omp simd
            for (int i = 0; i < 4; i++)
            {
                outp[i].raw[swizzleMask.x] = normals[i].x;
                outp[i].raw[swizzleMask.y] = normals[i].y;
                outp[i].raw[swizzleMask.z] = 0.0f;
                outp[i].raw[swizzleMask.w] = 1.0f;
            }
        }

        for (int i = 0; i < blockCount; i++)
        {
            imageIn[u + i] = FLOAT4_TO_PIX(imageIn, outp[i]);
        }
    }
}

static inline void HemiOct_ResolveVar(fix16v4* imageIn, const int2 resIn, const ivec4s swizzleMask)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for
    for (long u = 0; u < pixelCount; u += 4)
    {
        fix16v4 pixels[4];
        int blockCount = min(pixelCount - u, 4);
        for (int i = 0; i < blockCount; i++)
        {
            pixels[i] = imageIn[u + i];
        }

        vec4s fpixels[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            fpixels[i] = PIX_TO_FLOAT4(pixels[i]);
        }

        vec2s normals[4];

        float roughness[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            float x = 2.0 * fpixels[i].raw[swizzleMask.x] - 1.0;
            float y = 2.0 * fpixels[i].raw[swizzleMask.y] - 1.0;
            float z = sqrt(1.0 - fminf(x * x + y * y, 1.0));
            normals[i] = VecToHemiOct(x,y,z); 
            // variance of the normals on each axis. The values stored at swizzleMask z and w are the averages of x*x and y*y
            float varianceX = fpixels[i].raw[swizzleMask.z] - fpixels[i].raw[swizzleMask.x] * fpixels[i].raw[swizzleMask.x];
            float varianceY = fpixels[i].raw[swizzleMask.w] - fpixels[i].raw[swizzleMask.y] * fpixels[i].raw[swizzleMask.y];
            roughness[i] = sqrt(varianceX * varianceX + varianceY * varianceY);
        }

        vec4s outp[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            outp[i].raw[swizzleMask.x] = normals[i].x;
            outp[i].raw[swizzleMask.y] = normals[i].y;
            outp[i].raw[swizzleMask.z] = roughness[i];
            outp[i].raw[swizzleMask.w] = 0.0f; // unused;
        }
#pragma omp simd
        for (int i = 0; i < blockCount; i++)
        {
            imageIn[u + i] = FLOAT4_TO_PIX(imageIn, outp[i]);
        }
    }
}

static inline void ConvertFix16v4ToHalf4(fix16v4* imageIn, const int2 resIn)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for nowait
    for (long u = 0; u < pixelCount; u += 4)
    {
        fix16v4 pixels[4];
        int blockCount = min(pixelCount - u, 4);
        for (int i = 0; i < blockCount; i++)
        {
            pixels[i] = imageIn[u + i];
        }

        vec4s fpixels[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            fpixels[i] = PIX_TO_FLOAT4(pixels[i]);
        }

        half4* halfImage = (half4*)imageIn; // fix16v4 same size as half4, convert in place
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            halfImage[u + i] = FLOAT4_TO_PIX(halfImage, fpixels[i]);
        }
    }
}


static inline void ConvertFix16v3ToHalf3(fix16v3* imageIn, const int2 resIn)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for nowait
    for (long u = 0; u < pixelCount; u += 4)
    {
        fix16v3 pixels[4];
        int blockCount = min(pixelCount - u, 4);
        for (int i = 0; i < blockCount; i++)
        {
            pixels[i] = imageIn[u + i];
        }

        vec4s fpixels[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            fpixels[i] = PIX_TO_FLOAT4(pixels[i]);
        }

        half3* halfImage = (half3*)imageIn; // fix16v4 same size as half4, convert in place
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            halfImage[u + i] = FLOAT4_TO_PIX(halfImage, fpixels[i]);
        }
    }
}

void GenMips_Pow2_Normal_HemiOct_Roughness(fix16v4** mipChain, const int2* mipResolutions, int mipCount, const bool doResolve)
{
    const ivec4s swizzleMask = { 0, 1, 2, 3 };
    int threadCount = omp_get_num_procs();
    omp_set_num_threads(threadCount);
    printf("Thread Count: %d\n", threadCount);
    printf("Mip Count: %d\n", mipCount);
    #pragma omp parallel shared(mipChain, mipResolutions, mipCount, swizzleMask, doResolve)
    {
        Normalize_Var(mipChain[0], mipResolutions[0], swizzleMask, true);

        for (int mIdx = 1; mIdx < mipCount; mIdx++)
        {
            Pow2Mip_Box_Normal_Var(mipChain[mIdx - 1], mipResolutions[mIdx - 1], mipChain[mIdx], mipResolutions[mIdx], swizzleMask);
           // printf("Finished mip %d\n", mIdx);
        }
        
        if (doResolve)
        {
            for (int mIdx = 0; mIdx < mipCount; mIdx++)
            {
                HemiOct_ResolveVar(mipChain[mIdx], mipResolutions[mIdx], swizzleMask);
            }
        }
    }
}

void Pow2Mip_Box_Half2(half2* imageIn, const int2 resIn, half2* imageOut, const int2 resOut)
{
    int threadCount = omp_get_num_procs();
    omp_set_num_threads(threadCount);
#pragma omp parallel shared(imageIn, resIn, imageOut, resOut)
#pragma omp for

    for (int rowIdx = 0; rowIdx < resOut.y; rowIdx++)
    {
        int rowIn1Ptr = resIn.x * (2 * rowIdx);
        int rowIn2Ptr = rowIdx < (resOut.y - 1) ? rowIn1Ptr + resIn.x : rowIn1Ptr;
        int rowOutPtr = resOut.x * rowIdx;

        for (int u = 0; u < resOut.x; u++)
        {
            int xOffset = 2 * u;
            int xOffset2 = xOffset < (resIn.x - 1) ? xOffset + 1 : xOffset;
            half2 pixels[4] =
            {
                imageIn[rowIn1Ptr + xOffset],
                imageIn[rowIn1Ptr + xOffset2],
                imageIn[rowIn2Ptr + xOffset],
                imageIn[rowIn2Ptr + xOffset2]
            };


            vec2s fpixels[4];
#pragma omp simd
            for (int i = 0; i < 4; i++)
            {
                fpixels[i] = Half2ToFloat2(pixels[i]);
            }

            vec2s outp;
            outp.x = 0.25 * (fpixels[0].x + fpixels[1].x + fpixels[2].x + fpixels[3].x);
            outp.y = 0.25 * (fpixels[0].y + fpixels[1].y + fpixels[2].y + fpixels[3].y);

            imageOut[rowOutPtr + u] = Float2ToHalf2(outp);
        }
    }
}

TXPErrorCode YFlipImage(void* image, size_t rowSize, int rowCount)
{
    void* tempRow = malloc(rowSize);
    if (tempRow == NULL) return TXP_RETURN_ALLOC_FAILED;

    void* lastRow = image + (rowCount - 1) * rowSize;
    for (int row = 0; row < rowCount / 2; row++)
    {
        memcpy(tempRow, image + row * rowSize, rowSize);
        memcpy(image + row * rowSize, lastRow - row * rowSize, rowSize);
        memcpy(lastRow - row * rowSize, tempRow, rowSize);
    }
    free(tempRow);

    return TXP_RETURN_SUCCESS;
}

#define PIX_TYPE_IN fix16v4
#define PIX_TYPE_OUT fix16v4
DLLEXPORT void STDCALL TestSIMD(fix8v4* imageIn, int totalPixels, vec4s* imageOut)
{
    ivec4s swizzle = { 0, 1, 2, 3 };
#pragma omp simd
    for (int i = 0; i < totalPixels; i++)
    {
        vec4s fPixel = Fix8v4ToFloat4(imageIn[i]);
        //{
        //    fix8v4 value = imageIn[i];
        //    vec4s output =
        //    {
        //        Fix8ToFloat(value.r),
        //        Fix8ToFloat(value.g),
        //        Fix8ToFloat(value.b),
        //        Fix8ToFloat(value.a)
        //    };
        //    fPixel = output;
        //}
        //{
        //    Fix8ToFloat(imageIn[i].r),
        //    Fix8ToFloat(imageIn[i].g),
        //    Fix8ToFloat(imageIn[i].b),
        //    Fix8ToFloat(imageIn[i].a)
        //};
        //float x = 2.0 * fPixel.raw[swizzle.x] - 1.0;
        //float y = 2.0 * fPixel.raw[swizzle.y] - 1.0;
        //vec4s normal = HemiOctToVec(x, y);
        //vec4s outPixel = { .x = 0.5f, .y = 0.5f, .z = 0.5f, .w = 1.0f };
        //glm_vec4_muladds(normal.raw, 0.5f, outPixel.raw);
        imageOut[i] = fPixel;//FLOAT4_TO_PIX(imageOut, &outPixel);
    }
}
#undef PIX_TYPE_IN 
#undef PIX_TYPE_OUT