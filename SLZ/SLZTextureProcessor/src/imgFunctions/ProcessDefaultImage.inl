
#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)
#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)

#if !defined(PIX_TYPE_IN)

#define PIX_TYPE_IN fix8
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN fix8

#define PIX_TYPE_IN fix8v2
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix8v3
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix8v4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v2
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v3
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN fix16v4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN half
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half2
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half3
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN half4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN

#define PIX_TYPE_IN float
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float2
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float3
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN 

#define PIX_TYPE_IN float4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_IN


#elif !defined(PIX_TYPE_OUT)

#define PIX_TYPE_OUT fix8v4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT fix16v4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_OUT

#define PIX_TYPE_OUT half4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_OUT 

#define PIX_TYPE_OUT float4
#include "imgFunctions/ProcessDefaultImage.inl"
#undef PIX_TYPE_OUT

#else



#endif
TXPErrorCode CCATN(ReadImageToTexture_, PIX_TYPE_IN, PIX_TYPE_OUT)(void* imageIn, void* imageOut)
{

}

#undef CCAT2
#undef CCAT
#undef CCATN2
#undef CCATN