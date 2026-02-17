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

/** @brief Complete processing of a normal or detail map image. Can generate mips, re-encode 
 * normal directions to hemi-oct, and/or generate geometric roughness information.
 *
 * @param[in] image Source normal map image 
 * @param imageRes Resolution of the source image
 * @param[out] Pointer to a TxpTex2D which will be written 
 * @param swizzleMaskIn
 *      Vector indicating the index of each component in the input image.
 *      x: normal X, y: normal Y, z: geometric roughness or detail smoothness,
 *      w: detail AO
 * @param swizzleMaskOut
 *      Vector indicating the index of each component in the output image.
 *      x: normal X, y: normal Y, z: geometric roughness or detail smoothness,
 *      w: detail AOO
 * @param detailMap Flag indicating if this texture is to be treated as a
 *      detail map. Detail maps are always DXTnm (alpha-green), the red 
 *      channel is preserved, and geometric roughness is mixed with the
 *      overlay-blend smoothness stored in the blue channel
 * @param hemiOct Flag indicating if the normal vector gets remapped to hemi-
 *      octahedral coordinates
 * @param geoRoughness Flag indicating if geometric roughness information
        should be calculated and added.
 * @param genMips Flag indicating if mips should be generated. 
 *      TODO: remove this, we can just check if the output TxpTex2D has more than 1 mip
 */

TXPErrorCode CCATN(ProcessNormalMap_, PIX_TYPE_IN, PIX_TYPE_OUT)(
    void* image, int2 imageRes, TxpTex2D* output,
    ivec4s swizzleMaskIn, ivec4s swizzleMaskOut,
    int yFlip, int detailMap, int hemiOct, float geoRoughnessStr, int genMips)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    // Geometric roughness is calculated as a part of the mips,
    // If there's no mips don't even try to calculate it
    if (!genMips)
    {
        geoRoughnessStr = 0.0f;
    }

    //PIX_TYPE_IN dummy;
    //PIX_TYPE_OUT dummy2;

    //printf("Used %d In\n", TYPE_TO_FMT(&dummy));
    //printf("Used %d Out\n", TYPE_TO_FMT(&dummy2));
   
   
    int2 desiredRes = output->resolution[0];
    TxpTex2D* varianceBuffer = NULL;
    int varianceMipStart = 0;
    int getHighVar = 0;

    // Unity uses OpenGL style coordinates where the bottom left is 0,0
    // Literally everything else uses the top left as 0,0, so we have to
    // vertically flip the image otherwise it will appear upside-down in
    // unity
    if (yFlip)
    {
        err = YFlipImage(image, sizeof(PIX_TYPE_IN) * imageRes.x, imageRes.y);
        TXP_HANDLE_ERROR(err, exit)
    }

    if (geoRoughnessStr > 0.0f)
    {
        int2 upscaleRes = {};
       
        if (desiredRes.x > desiredRes.y)
        {
            //double aspect = (double)desiredRes.x / (double)desiredRes.y;
            upscaleRes.y = (int)ceilPow2((unsigned int)imageRes.y);
            upscaleRes.x = desiredRes.x * upscaleRes.y / desiredRes.y ;
        }
        else
        {
            //double aspect = (double)desiredRes.y / (double)desiredRes.x;
            upscaleRes.x = (int)ceilPow2((unsigned int)imageRes.x);
            upscaleRes.y = desiredRes.y * upscaleRes.x / desiredRes.x;
        }

        getHighVar = upscaleRes.x >= (2 * desiredRes.x) && upscaleRes.y >= (2 * desiredRes.y);
        if (!getHighVar) upscaleRes = desiredRes;
        err = CreateTexture2D(&varianceBuffer, upscaleRes.x, upscaleRes.y, FMT_RGHalf, 0);
        TXP_HANDLE_ERROR(err, cleanupVar)

        if (getHighVar)
        {
            //printf("upscale variance res: %d x %d\n", upscaleRes.x, upscaleRes.y);
            varianceMipStart = round(log2((double)upscaleRes.x / (double)desiredRes.x));

            PIX_TYPE_OUT* scaleBuffer = (PIX_TYPE_OUT*)malloc(sizeof(PIX_TYPE_OUT) * upscaleRes.x * upscaleRes.y);
            if (scaleBuffer == NULL) { err = TXP_RETURN_ALLOC_FAILED; goto cleanupVar; }

            // Upscale the source image to the next highest power of 2 with mitchell
            // TODO: A much, much better way to do this would be to directly calculate 
            // a box-filtered average of the X^2 and Y^2 values from the input at the 
            // resolution of the output. I'm lazy and reusing mipping code to calculate
            // the averages right now which requires a power of 2 input.
            // There's potential issues with non-power of 2 input images though, 
            // I'd probably have to do a smooth box filter and weighted average based on
            // pixel coverage
            err = CCATN(ScaleImageMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)((PIX_TYPE_IN*)image, imageRes, scaleBuffer, upscaleRes);
            if (err != TXP_RETURN_SUCCESS) { free(scaleBuffer); goto cleanupVar; }

            // Downscale the upscaled image into the target's mip 0 with box filtering. 
            // Calculating geometric roughness requires that each mip level contain the average
            // of the X and Y normal vector components. If we directly scale to the target res
            // with mitchell, X and Y will not be the true average values which will result in
            // erroneously high roughnesses.
            CCATN(ScaleImageBox_, PIX_TYPE_OUT, PIX_TYPE_OUT)((PIX_TYPE_OUT*)scaleBuffer, upscaleRes, (PIX_TYPE_OUT*)output->mips[0], output->resolution[0]);

            // Calculate a mip chain containing the average X^2 and Y^2 components of the normals at each level
            // Used for calculating the variance of the X and Y, which in turn is used for calculating the 
            // geometric roughness.
            CCAT(GenVarianceMips_Pow2_, PIX_TYPE_OUT)(
                scaleBuffer,
                (half2**)varianceBuffer->mips,
                swizzleMaskIn,
                varianceBuffer->resolution,
                varianceBuffer->mipCount);

            free(scaleBuffer);
        }
        else
        {
            err = CCATN(ScaleImageMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)((PIX_TYPE_IN*)image, imageRes, (PIX_TYPE_OUT*)output->mips[0], output->resolution[0]);
            TXP_HANDLE_ERROR(err, cleanupVar)

            CCAT(GenVarianceMips_Pow2_, PIX_TYPE_OUT)(
                (PIX_TYPE_OUT*)output->mips[0],
                (half2**)varianceBuffer->mips,
                swizzleMaskIn,
                varianceBuffer->resolution,
                varianceBuffer->mipCount);
        }
    }
    else
    {
        err = CCATN(ScaleImageMitchell_, PIX_TYPE_IN, PIX_TYPE_OUT)((PIX_TYPE_IN*)image, imageRes, (PIX_TYPE_OUT*)output->mips[0], output->resolution[0]);
        TXP_HANDLE_ERROR(err, cleanupVar)
    }

    half2** varianceChain = NULL;
    if (varianceBuffer != NULL) varianceChain = (half2**)&(varianceBuffer->mips[varianceMipStart]);
    CCAT(GenMips_Pow2_Normal_, PIX_TYPE_OUT)(
        (PIX_TYPE_OUT**)output->mips,
        varianceChain,
        swizzleMaskIn,
        swizzleMaskOut,
        output->resolution,
        output->mipCount,
        hemiOct,
        geoRoughnessStr,
        detailMap
        );

    cleanupVar:
        DisposeTexture2D(varianceBuffer);
    exit:
        return err;
}


#undef CCAT2
#undef CCAT
#undef CCATN2
#undef CCATN