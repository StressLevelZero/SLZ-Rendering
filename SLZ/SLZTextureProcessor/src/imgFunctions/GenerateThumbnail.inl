#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)

#ifndef PIX_TYPE_IN
#error Template type PIX_TYPE_IN not defined
#endif

#ifndef PIX_TYPE_OUT
#error Template type PIX_TYPE_OUT not defined
#endif

void CCATN(ConvertThumbnail_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, PIX_TYPE_OUT* imageOut, int2 resolution, ivec4s swizzle, int isNormal, int hemiOct)
{

	long totalPixels = (long)resolution.x * (long)resolution.y;
	// Thumbnail is very small, don't bother multithreading
	//#pragma omp parallel for shared(totalPixels, imageIn, imageOut)
	if (isNormal)
	{
		if (hemiOct)
		{
#pragma omp simd
			for (long i = 0; i < totalPixels; i++)
			{
				vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
				float x = 2.0 * fPixel.raw[swizzle.x] - 1.0;
				float y = 2.0 * fPixel.raw[swizzle.y] - 1.0;
				vec4s normal = HemiOctToVec(x, y);
				vec4s outPixel = { .x = 0.5f, .y = 0.5f, .z = 0.5f, .w = 1.0f };
				glm_vec4_muladds(normal.raw, 0.5f, outPixel.raw);
				imageOut[i] = FLOAT4_TO_PIX(imageOut, outPixel);
			}
		}
		else
		{
#pragma omp simd
			for (long i = 0; i < totalPixels; i++)
			{
				vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
				float x = 2.0 * fPixel.raw[swizzle.x] - 1.0;
				float y = 2.0 * fPixel.raw[swizzle.y] - 1.0;
				float z = sqrtf(fmaxf(1.0f - (x * x) - (y * y), 0.0f));
				vec4s normal = { .x = x, .y = y, .z = z, 1.0f };
				vec4s outPixel = { .x = 0.5f, .y = 0.5f, .z = 0.5f, .w = 1.0f };
				glm_vec4_muladds(normal.raw, 0.5f, outPixel.raw);
				imageOut[i] = FLOAT4_TO_PIX(imageOut, outPixel);
			}
		}
	}
	else
	{
#pragma omp simd
		for (long i = 0; i < totalPixels; i++)
		{
			vec4s fPixel = PIX_TO_FLOAT4(imageIn[i]);
			
			vec4s outPixel = {
				.x = fPixel.raw[swizzle.x],
				.y = fPixel.raw[swizzle.y],
				.z = fPixel.raw[swizzle.z],
				.w = 1.0f
			};
			imageOut[i] = FLOAT4_TO_PIX(imageOut, outPixel);
		}
	}
}

TXPErrorCode CCATN(GenerateThumbnail_, PIX_TYPE_IN, PIX_TYPE_OUT)(void* imageIn, const int2 resIn, void* imageOut, const int2 resOut, ivec4s swizzle, int isNormal, int hemiOct)
{
	TXPErrorCode err = TXP_RETURN_SUCCESS;
	if (resIn.x != resOut.x || resIn.y != resOut.y)
	{
		PIX_TYPE_IN* tempIcon = malloc(sizeof(PIX_TYPE_IN) * resIn.x * resIn.y);
		TXPErrorCode err = CCATN(ScaleImageMitchell_, PIX_TYPE_IN, PIX_TYPE_IN)((PIX_TYPE_IN*)imageIn, resIn, tempIcon, resOut);
		if (err) goto cleanup;
		CCATN(ConvertThumbnail_, PIX_TYPE_IN, PIX_TYPE_OUT)(tempIcon, imageOut, resOut, swizzle, isNormal, hemiOct);
	cleanup:
		free(tempIcon);
	}
	else
	{
		CCATN(ConvertThumbnail_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, imageOut, resOut, swizzle, isNormal, hemiOct);
	}
	return err;
}


#undef CCAT2
#undef CCAT
#undef CCATN2
#undef CCATN