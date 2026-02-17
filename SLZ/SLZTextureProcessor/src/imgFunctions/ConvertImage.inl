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
#ifdef DEBUG_PRINT
	printf("Running ConvertImage\n");
#endif
	long totalPixels = (long)resolution.x * (long)resolution.y;
#pragma omp parallel for shared(totalPixels, imageIn, imageOut)
	for (long i = 0; i < totalPixels; i++)
	{
		vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
		vec4s outPixel = glms_vec4_add(glms_mat4_mulv(swizzleMat, fPixel), swizzleAdd);
		imageOut[i] = FLOAT4_TO_PIX(imageOut, outPixel);
	}
}

#undef CCATN2 
#undef CCATN