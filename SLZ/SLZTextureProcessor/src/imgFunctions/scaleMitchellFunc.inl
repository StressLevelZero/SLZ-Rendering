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

/* @brief Scales a single row of an input image on X using Mitchell-Netravali
 *        interpolation and stores it in the given output buffer 
 * 
 * @param rowIn         Pointer to the start of the row to be scaled
 * @param resolutionIn  width of the input image
 * @param[out] rowOut   Pointer to the start of the row in the output image
 * @param resolutionOut width of the output image
 */
static inline void CCATN(ScaleRowMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* rowIn, const int resolutionIn, PIX_TYPE_OUT* rowOut, const int resolutionOut)
// static inline void ScaleXMitchell_TYPE (TYPE* rowIn, const int resolutionIn, TYPE* rowOut, const int resolutionOut)
{
	//#include "imgFunctions/scaleXMitchell.inl" 
    double resInFl = (double)resolutionIn;
    double rcpResOutFl = 1.0 / (double)resolutionOut;

    // the center of a pixel is offset from its coordinates by 1/2 of a pixel on each axis.
    // This is the offset of the center of the new smaller pixels in the space of the original
    // image's coordinates
    double outPixWidth = fmax(1.0, resInFl * rcpResOutFl);
    double rcpOutPixWidth = 1.0 / outPixWidth;
    double pixOffset = 0.5 * resInFl * rcpResOutFl - 0.5;
    int maxX = resolutionIn - 1;
    for (int x = 0; x < resolutionOut; x++)
    {
        double u = ((double)x * rcpResOutFl) * resInFl + pixOffset;
        int rangeStart = ceil(u - 2.0 * outPixWidth);
        int rangeEnd = floor(u + 2.0 * outPixWidth);
        rangeEnd = imin(rangeEnd, maxX);

        int rangeCount = rangeEnd - rangeStart;
        vec4s finalColor = { 0 };
        double totalWeight = 0;

        // loop over groups of 4 pixels for vector operations 
        for (int vIdx = rangeStart; vIdx <= rangeEnd; vIdx += 4)
        {
            ivec4 supportCoords;
            
#pragma omp simd
            for (int sIdx = 0; sIdx < 4; sIdx++)
            {
                supportCoords[sIdx] = imax(0, vIdx + sIdx);
            }
#pragma omp simd
            for (int sIdx = 0; sIdx < 4; sIdx++)
            {
                supportCoords[sIdx] = imin(supportCoords[sIdx], rangeEnd);
            }
            
            vec4s mitchelWeights;
#pragma omp simd
            for (int mIdx = 0; mIdx < 4; mIdx++)
            {
                mitchelWeights.raw[mIdx] = MitchellNetravaliWeight(fabs((double)supportCoords[mIdx] - u) * rcpOutPixWidth);
            }

            PIX_TYPE_IN mitchellSupports[4] = {
                rowIn[supportCoords[0]],
                rowIn[supportCoords[1]],
                rowIn[supportCoords[2]],
                rowIn[supportCoords[3]]
            };

            vec4s floatSupports[4] = {
                 PIX_TO_FLOAT4(mitchellSupports[0]),
                 PIX_TO_FLOAT4(mitchellSupports[1]),
                 PIX_TO_FLOAT4(mitchellSupports[2]),
                 PIX_TO_FLOAT4(mitchellSupports[3])
            };

            mat4s supportMatrix = glms_mat4_make((float*)floatSupports);
            finalColor = glms_vec4_add(finalColor, glms_mat4_mulv(supportMatrix, mitchelWeights));
            totalWeight += mitchelWeights.x + mitchelWeights.y + mitchelWeights.z + mitchelWeights.w;
        }
        finalColor = glms_vec4_scale(finalColor, 1.0 / totalWeight);

        rowOut[x] = FLOAT4_TO_PIX(rowOut, finalColor);
    }
}


/* @brief Scales on Y 8 columns of an input image using Mitchell-Netravali
 *        interpolation and stores it in the given output buffer.
 *
 * @param imageIn       Pointer to the input image 
 * @param resolutionIn  width and height of the input image
 * @param[out] imageOut Pointer to the output image
 * @param resolutionOut width and height of the output image
 * @param blockIdx      the x coordinate divided by 8 of the start of the 8
 *                      column-wide block to scale
 */
static inline void CCATN(ScaleColumnBlockMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, const int2 resolutionIn, PIX_TYPE_OUT* imageOut, const int2 resolutionOut, int blockIdx)
// static inline void ScaleYMitchell_TYPE (TYPE* imageIn, const int2 resolutionIn, TYPE* imageOut, const int2 resolutionOut, int blockIdx)
{
    //#include "imgFunctions/scaleYMitchell.inl" 
    double resInFl = (double)resolutionIn.y;
    double rcpResOutFl = 1.0 / (double)resolutionOut.y;

    // the center of a pixel is offset from its coordinates by 1/2 of a pixel on each axis.
    // This is the offset of the center of the output pixels in the space of the original
    // image's coordinates
    double pixOffset = 0.5 * (resInFl * rcpResOutFl) - 0.5;

    // height of the scaled pixel in terms of the original resolution. If upscaling, the height is clamped to 1
    double outPixHeight = fmax(1.0, resInFl * rcpResOutFl);
    double rcpOutPixHeight = 1.0 / outPixHeight;

    // Compute 8 wide rows of pixels at once to increase cache coherency. Since the image is
    // row-major, grabbing pixels along the Y-axis is basically a guaranteed cache miss.
    int rowSize = imin(8, (resolutionIn.x - blockIdx * 8));

    for (int y = 0; y < resolutionOut.y; y++)
    {
        double v = ((double)y * rcpResOutFl) * resInFl + pixOffset;

        int rangeStart = ceil(v - 2.0 * outPixHeight);
        int rangeEnd = floor(v + 2.0 * outPixHeight);

        float totalWeight = 0.0f;
        vec4s rowTotal[8] = { 0 };

        for (int i = rangeStart; i <= rangeEnd; i++)
        {
            PIX_TYPE_IN row[8];
            int ptr = imin(imax(0, i), resolutionIn.y - 1) * resolutionIn.x + 8 * blockIdx;

            for (int r = 0; r < rowSize; r++)
            {
                row[r] = imageIn[ptr + r];
            }

            float weight = MitchellNetravaliWeight(fabs(v - (float)i) * rcpOutPixHeight);
            vec4s rowFloat[8];

            for (int r = 0; r < 8; r++)
            {
                rowFloat[r] = PIX_TO_FLOAT4(row[r]);
            }

            for (int r = 0; r < 8; r++)
            {
                glm_vec4_muladds(rowFloat[r].raw, weight, rowTotal[r].raw);
            }

            totalWeight += weight;
        }

        float rcpTotalWeight = 1.0f / totalWeight;
        for (int r = 0; r < rowSize; r++)
        {
            rowTotal[r] = glms_vec4_scale(rowTotal[r], rcpTotalWeight);
        }
        int ptrOut = y * resolutionOut.x + (8 * blockIdx);
        for (int r = 0; r < rowSize; r++)
        {
            imageOut[ptrOut + r] = FLOAT4_TO_PIX(imageOut, rowTotal[r]);
        }
    }
}

/* @brief Scales an image only on X using Mitchell-Netravali interpolation
 *
 * @param imageIn       Pointer to the input image
 * @param resolutionIn  width and height of the input image
 * @param imageOut        Pointer to the output image
 * @param resolutionOut width and height of the output image
 */
static void CCATN(ScaleImageMitchell_X_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, const int2 resolutionIn, PIX_TYPE_OUT* imageOut, const int2 resolutionOut)
// static void ScaleImageMitchell_X_TYPE (TYPE* imageIn, const int2 resolutionIn, TYPE* imageOut, const int2 resolutionOut)
{
#pragma omp parallel shared(imageIn, resolutionIn, imageOut, resolutionOut)
    {
#pragma omp for 
        for (int row = 0; row < resolutionIn.y; row++)
        {
            CCATN(ScaleRowMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(&imageIn[row * resolutionIn.x], resolutionIn.x, &imageOut[row * resolutionOut.x], resolutionOut.x);
        }
    }
}

/* @brief Scales an image only on Y using Mitchell-Netravali interpolation
 *
 * @param imageIn       Pointer to the input image
 * @param resolutionIn  width and height of the input image
 * @param imageOut        Pointer to the output image
 * @param resolutionOut width and height of the output image
 */
static void CCATN(ScaleImageMitchell_Y_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, const int2 resolutionIn, PIX_TYPE_OUT* imageOut, const int2 resolutionOut)
// static void ScaleImageMitchell_Y_TYPE (TYPE* imageIn, const int2 resolutionIn, TYPE* imageOut, const int2 resolutionOut)
{
    const int columnBlocks = (resolutionOut.x + 7) / 8; // ceil(resolutionOut.x / 8)
#pragma omp parallel shared(imageIn, resolutionIn, imageOut, resolutionOut, columnBlocks)
    {
#pragma omp for 
        for (int columnBlock = 0; columnBlock < columnBlocks; columnBlock++)
        {
            CCATN(ScaleColumnBlockMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, resolutionIn, imageOut, resolutionOut, columnBlock);
        }
    }
}

/* @brief Scales an image on both X and Y using Mitchell-Netravali filtering
 *
 * @param imageIn       Pointer to the input image
 * @param resolutionIn  width and height of the input image
 * @param imageOut        Pointer to the output image
 * @param resolutionOut width and height of the output image
 */
static TXPErrorCode CCATN(ScaleImageMitchell_XY_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, const int2 resolutionIn, PIX_TYPE_OUT* imageOut, const int2 resolutionOut)
// static void ScaleImageMitchell_XY_TYPE (TYPE* imageIn, const int2 resolutionIn, TYPE* imageOut, const int2 resolutionOut)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    const int columnBlocks = (resolutionOut.x + 7) / 8; // ceil(resolutionOut.x / 8)
    const int2 intermediateRes = { resolutionOut.x, resolutionIn.y };
    PIX_TYPE_IN* intermediateImg = malloc(sizeof(imageIn[0]) * intermediateRes.x * intermediateRes.y);
    if (intermediateImg == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto exit; }

#pragma omp parallel shared(imageIn, resolutionIn, imageOut, resolutionOut, columnBlocks, intermediateImg)
    {
#pragma omp for 
        for (int row = 0; row < intermediateRes.y; row++)
        {
            CCATN(ScaleRowMitchell_, PIX_TYPE_IN, PIX_TYPE_IN)(&imageIn[row * resolutionIn.x], resolutionIn.x, &intermediateImg[row * intermediateRes.x], intermediateRes.x);
        }

#pragma omp for 
        for (int columnBlock = 0; columnBlock < columnBlocks; columnBlock++)
        {
            CCATN(ScaleColumnBlockMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(intermediateImg, intermediateRes, imageOut, resolutionOut, columnBlock);
        }
    }

    free(intermediateImg);
    exit:
        return err;
}



/* @brief Scales an image using Mitchell-Netravali filtering and stores it in the given output image
 *
 * @param imageIn       Pointer to the input image
 * @param resolutionIn  Width and height of the input image
 * @param[out] imageOut Pointer to the output image
 * @param resolutionOut Width and height of the output image
 */
TXPErrorCode CCATN(ScaleImageMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)(PIX_TYPE_IN* imageIn, const int2 resolutionIn, PIX_TYPE_OUT* imageOut, const int2 resolutionOut)
// void ScaleImageMitchell_TYPE (TYPE* imageIn, const int2 resolutionIn, TYPE* imageOut, const int2 resolutionOut)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    int scaleType =
        (IMG_SCALE_X * (resolutionOut.x != resolutionIn.x)) |
        (IMG_SCALE_Y * (resolutionOut.y != resolutionIn.y));

    int threadCount = omp_get_num_procs();
    omp_set_num_threads(threadCount);
    switch (scaleType)
    {
    case (IMG_SCALE_EQUAL):
        CCATN(ConvertImage_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, imageOut, resolutionIn);
        break;
    case (IMG_SCALE_X):
        CCATN(ScaleImageMitchell_X_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, resolutionIn, imageOut, resolutionOut);
        break;
    case (IMG_SCALE_Y):
        CCATN(ScaleImageMitchell_Y_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, resolutionIn, imageOut, resolutionOut);
        break;
    case (IMG_SCALE_X | IMG_SCALE_Y):
        err = CCATN(ScaleImageMitchell_XY_, PIX_TYPE_IN, PIX_TYPE_OUT)(imageIn, resolutionIn, imageOut, resolutionOut);
        break;
    }
    return err;
}

#undef CCAT2
#undef CCAT
#undef CCATN2
#undef CCATN