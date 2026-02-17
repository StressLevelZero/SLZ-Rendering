#ifndef SLZ_TEXTURE_PROCESSOR_OPENMP
#define SLZ_TEXTURE_PROCESSOR_OPENMP

#include "Platform.h"
#include <texture.h>

DLLEXPORT void STDCALL TestScaleImage(fix8v4* imageIn, const int2 resolutionIn, fix8v4* imageOut, const int2 resolutionOut);

#define DECLARE_SCALE_IMG_MITCHELL(TIN, TOUT) TXPErrorCode ScaleImageMitchell_##TIN##_##TOUT (TIN* imageIn, const int2 resolutionIn, TOUT* imageOut, const int2 resolutionOut)
DECLARE_SCALE_IMG_MITCHELL(fix8v3, fix8v4);
DECLARE_SCALE_IMG_MITCHELL(fix8v4, fix8v4);
DECLARE_SCALE_IMG_MITCHELL(fix16v3, fix16v4);
DECLARE_SCALE_IMG_MITCHELL(fix16v4, fix16v4);
DECLARE_SCALE_IMG_MITCHELL(half3, half4);
DECLARE_SCALE_IMG_MITCHELL(half4, half4);
#undef DECLARE_SCALE_IMG_MITCHELL

void Pow2Mip_Box_Half2(half2* imageIn, const int2 resIn, half2* imageOut, const int2 resOut);
void GenMips_Pow2_Normal_HemiOct_Roughness(fix16v4** mipChain, const int2* mipResolutions, int mipCount, const bool doResolve);
void InitVarianceDirect_fix16v4(fix16v4* imageIn, half2* varianceBuf, const int2 resIn, const ivec4s swizzleMaskIn);
#define DECLARE_GENMIPS_POW2_NORMAL(TIN) void GenMips_Pow2_Normal_##TIN( \
	TIN** mipChain, \
	half2** varianceMips, \
	const ivec4s swizzleMaskIn, \
	const ivec4s swizzleMaskOut, \
	const int2* mipResolutions, \
	int mipCount, \
	const int hemiOct, \
	const float geoRoughnessStr, \
	const int detailMap \
	) \

DECLARE_GENMIPS_POW2_NORMAL(fix16v4);

#undef DECLARE_GENMIPS_POW2_NORMAL


#define DECLARE_GENVARIANCEMIPS_POW2(TIN) void GenVarianceMips_Pow2_##TIN( \
		TIN* mip0, \
		half2** varianceMips, \
		const ivec4s swizzleMaskIn, \
		const int2* mipResolutions, \
		int mipCount \
		) \

DECLARE_GENVARIANCEMIPS_POW2(fix16v4);

#undef DECLARE_GENVARIANCEMIPS_POW2

// Normal processing functions
#define DECLARE_PROCESSNORMAL(TIN, TOUT) TXPErrorCode ProcessNormalMap_##TIN##_##TOUT(void* image, int2 imageRes, TxpTex2D* output, ivec4s swizzleMaskIn, ivec4s swizzleMaskOut, int yFlip, int detailMap, int hemiOct, float geoRoughnessStr, int genMips)

DECLARE_PROCESSNORMAL(fix16v4, fix16v4);

#undef DECLARE_PROCESSNORMAL


TXPErrorCode ProcessNormalMap(TxpTextureFormat formatIn, TxpTextureFormat formatOut, void* image, int2 imageRes, TxpTex2D* output,
	ivec4s swizzleMaskIn, ivec4s swizzleMaskOut,
	int yFlip, int detailMap, int hemiOct, float geoRoughnessStr, int genMips);

TXPErrorCode CreateThumbnail(TxpTextureFormat formatIn, void* imageIn, int2 resIn, void* imageOut, int2 resOut, ivec4s swizzle, int isNormal, int hemiOct);


void ConvertImage_vec3s_fix16v3(vec3s* imageIn, fix16v3* imageOut, int2 resolution);
void ConvertImage_vec4s_fix16v4(vec4s* imageIn, fix16v4* imageOut, int2 resolution);
void ConvertImage_half3_fix16v3(half3* imageIn, fix16v3* imageOut, int2 resolution);
void ConvertImage_half4_fix16v4(half4* imageIn, fix16v4* imageOut, int2 resolution);

#endif