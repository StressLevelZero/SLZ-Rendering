#if !defined(PIX_TYPE_IN0)


#define PIX_TYPE_IN0 fix8v2
#define PIX_TYPE_OUT fix8v4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 fix8v3
#define PIX_TYPE_OUT fix8v4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 fix8v4
#define PIX_TYPE_OUT fix8v4
#define CREATE_INTERMEDIATE_SWITCH
#define PIX_TYPE_1_ENUM FMT_R8
#define PIX_TYPE_BASE fix8v
#include "imgFunctions/CombineAoSmNrm.inl"
#undef CREATE_INTERMEDIATE_SWITCH
#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_BASE
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 fix16v2
#define PIX_TYPE_OUT fix16v4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 fix16v3
#define PIX_TYPE_OUT fix16v4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 fix16v4
#define PIX_TYPE_OUT fix16v4
#define CREATE_INTERMEDIATE_SWITCH
#define PIX_TYPE_1_ENUM FMT_R16
#define PIX_TYPE_BASE fix16v
#include "imgFunctions/CombineAoSmNrm.inl"
#undef CREATE_INTERMEDIATE_SWITCH
#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_BASE
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 half2
#define PIX_TYPE_OUT half4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 half3
#define PIX_TYPE_OUT half4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 half4
#define PIX_TYPE_OUT half4
#define CREATE_INTERMEDIATE_SWITCH
#define PIX_TYPE_1_ENUM FMT_RHalf
#define PIX_TYPE_BASE half
#include "imgFunctions/CombineAoSmNrm.inl"
#undef CREATE_INTERMEDIATE_SWITCH
#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_BASE
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 


#define PIX_TYPE_IN0 float2
#define PIX_TYPE_OUT float4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 float3
#define PIX_TYPE_OUT float4
#include "imgFunctions/CombineAoSmNrm.inl"
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

#define PIX_TYPE_IN0 float4
#define PIX_TYPE_OUT float4
#define CREATE_INTERMEDIATE_SWITCH
#define PIX_TYPE_1_ENUM FMT_RHalf
#define PIX_TYPE_BASE float
#include "imgFunctions/CombineAoSmNrm.inl"
#undef CREATE_INTERMEDIATE_SWITCH
#undef PIX_TYPE_1_ENUM
#undef PIX_TYPE_BASE
#undef PIX_TYPE_IN0 
#undef PIX_TYPE_OUT 

TXPErrorCode CombineAoSmNrm(TxpTextureFormat outFormat, void* aoSmImageInOut, TxpTextureFormat nrmFormat, void* normalImageInOut, int2 resolution)
{
    switch (outFormat)
    {
    case FMT_RGBA8:     CombineAoSmNrm_fix8v4    (aoSmImageInOut, nrmFormat, normalImageInOut, resolution);    return TXP_RETURN_SUCCESS;
    case FMT_RGBA16:    CombineAoSmNrm_fix16v4   (aoSmImageInOut, nrmFormat, normalImageInOut, resolution);    return TXP_RETURN_SUCCESS;
    case FMT_RGBAHalf:  CombineAoSmNrm_half4     (aoSmImageInOut, nrmFormat, normalImageInOut, resolution);    return TXP_RETURN_SUCCESS;
    case FMT_RGBAFloat: CombineAoSmNrm_float4    (aoSmImageInOut, nrmFormat, normalImageInOut, resolution);    return TXP_RETURN_SUCCESS;
    default: return TXP_RETURN_INVALID_TEX_FORMAT;
    }
}

#else

#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)


void CCATN(CombineAoSmNrm_, PIX_TYPE_OUT, PIX_TYPE_IN0)(PIX_TYPE_OUT* aoSmImageInOut, PIX_TYPE_IN0* normalImage, int2 resolution)
{
    long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(aoSmImageInOut, normalImage)
    for (long i = 0; i < totalPixels; i++)
    {
        PIX_TYPE_IN0 normPixel = normalImage[i];
        PIX_TYPE_OUT aoSmPixel = aoSmImageInOut[i];
        
        aoSmImageInOut[i] = (PIX_TYPE_OUT){ aoSmPixel.y, normPixel.y, aoSmPixel.z, normPixel.x };
    }
}




#if defined(CREATE_INTERMEDIATE_SWITCH)

#if !defined(PIX_TYPE_OUT)
#error PIX_TYPE_OUT not defined!
#endif

#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

#define CCATM2(x,y,z,w) x##y##_##z##w
#define CCATM(x,y,z,w) CCATM2(x,y,z,w)

TXPErrorCode CCAT(CombineAoSmNrm_, PIX_TYPE_OUT)(PIX_TYPE_OUT* aoSmImageInOut, TxpTextureFormat nrmFormat, void* normalImage, int2 resolution)
{
    switch (nrmFormat)
    {
    case (PIX_TYPE_1_ENUM + 1): CCATM(CombineAoSmNrm_, PIX_TYPE_OUT, PIX_TYPE_BASE, 2) (aoSmImageInOut, normalImage, resolution);    return TXP_RETURN_SUCCESS;
    case (PIX_TYPE_1_ENUM + 2): CCATM(CombineAoSmNrm_, PIX_TYPE_OUT, PIX_TYPE_BASE, 3) (aoSmImageInOut, normalImage, resolution);    return TXP_RETURN_SUCCESS;
    case (PIX_TYPE_1_ENUM + 3): CCATM(CombineAoSmNrm_, PIX_TYPE_OUT, PIX_TYPE_BASE, 4) (aoSmImageInOut, normalImage, resolution);    return TXP_RETURN_SUCCESS;
    default: return TXP_RETURN_INVALID_TEX_FORMAT;
    }
}

#undef CCAT2
#undef CCAT

#undef CCATM2
#undef CCATM

#endif


#undef CCATN2 
#undef CCATN

#endif