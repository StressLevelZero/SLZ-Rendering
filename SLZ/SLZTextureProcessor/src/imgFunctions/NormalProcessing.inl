#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

#ifndef PIX_TYPE_IN
#error Template type PIX_TYPE_IN not defined
#endif


/** @brief Given a normal map, populate a second image of the same resolution
 *  with the square of the normal X and the normal Y. Mips of this image are
 *  used to calculate the variance of the normals for geometric roughness
 * 
 * @param[in]   imageIn     Normal/detail map to create a variance buffer for
 * @param[out]  varianceBuf Red-green half2 image of the same resolution as
 *      imageIn which will be populated with the square of the X and Y from
 *      imageIn
 * @param resIn Resolution of the image
 * @param swizzleMaskIn 
 *      Vector indicating the index of each component in the input image.
 *      x: normal X, y: normal Y, z: geometric roughness or detail smoothness, 
 *      w: detail AO
 */
inline void CCAT(InitVarianceDirect_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, half2* varianceBuf, const int2 resIn, const ivec4s swizzleMaskIn)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for
    for (long u = 0; u < pixelCount; u += 4)
    {
        PIX_TYPE_IN pixels[4];
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
            float x = 2.0 * fpixels[i].raw[swizzleMaskIn.x] - 1.0;
            float y = 2.0 * fpixels[i].raw[swizzleMaskIn.y] - 1.0;
            normals[i] = (vec4s){ x, y, sqrtf(fmaxf(1.0f - x * x - y * y, 0.0f)), 0 }; //HemiOctToVec(fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]);
            glm_vec3_normalize(normals[i].raw);
            normals[i].x = 0.5 * normals[i].x + 0.5;
            normals[i].y = 0.5 * normals[i].y + 0.5;
            //normals[i].z = 0.5 * normals[i].z + 0.5;
        }

        vec2s varOut[4];

#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            varOut[i].x = normals[i].x * normals[i].x;
            varOut[i].y = normals[i].y * normals[i].y;
        }

        for (int i = 0; i < blockCount; i++)
        {
            varianceBuf[u + i] = Float2ToHalf2(varOut[i]);
        }
    }
}


/** @brief Normalizes the vectors contained in a normal/detail map, and swizzles
 *  the components to match the desired output.
 *   
 * @param imageIn[in,out]   The normal or detail map
 * @param resIn             Resolution of the image
 * @param swizzleMaskIn 
 *      Vector indicating the index of each component in the input image.
 *      x: normal X, y: normal Y, z: geometric roughness or detail smoothness, 
 *      w: detail AO
 * @param swizzleMaskOut
 *      Vector indicating the index of each component in the output image.
 *      x: normal X, y: normal Y, z: geometric roughness or detail smoothness, 
 *      w: detail AO
 */
static inline void CCAT(InitNormalize_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, const int2 resIn, const ivec4s swizzleMaskIn, const ivec4s swizzleMaskOut, int preserveOtherChannels)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for
    for (long u = 0; u < pixelCount; u += 4)
    {
        PIX_TYPE_IN pixels[4];
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
            float x = 2.0 * fpixels[i].raw[swizzleMaskIn.x] - 1.0;
            float y = 2.0 * fpixels[i].raw[swizzleMaskIn.y] - 1.0;
            normals[i] = (vec4s){ x, y, sqrtf(fmaxf(1.0f - x * x - y * y, 0.0f)), 0 }; //HemiOctToVec(fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]);
            glm_vec3_normalize(normals[i].raw);
            normals[i].x = 0.5 * normals[i].x + 0.5;
            normals[i].y = 0.5 * normals[i].y + 0.5;
            //normals[i].z = 0.5 * normals[i].z + 0.5;
        }
        vec4s outp[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            outp[i].raw[swizzleMaskOut.x] = normals[i].x;
            outp[i].raw[swizzleMaskOut.y] = normals[i].y;
            outp[i].raw[swizzleMaskOut.z] = preserveOtherChannels != 0 ? fpixels[i].raw[swizzleMaskIn.z] : 0; // z is the blue channel, should be 0 if we're not a detail map
            outp[i].raw[swizzleMaskOut.w] = preserveOtherChannels != 0 ? fpixels[i].raw[swizzleMaskIn.w] : 1; // w is either the red or alpha channel, should be 1 if not a detail map
        }

        for (int i = 0; i < blockCount; i++)
        {
            imageIn[u + i] = FLOAT4_TO_PIX(imageIn, outp[i]);
        }
    }
}


/** @brief Calculates the next mip in the chain from a given mip image, assuming 
 *  the resolution of the mip is divisible by 2 on each axis. Downscales by a 
 *  factor of 2 using box filtering. Properly averages the normal X and Y by 
 *  averaging the full normal vectors and renormalizing.
 * 
 * @param imageIn   The previous mip level of the normal/detail map
 * @param resIn     Resolution of the previous mip level
 * @param imageOut  Memory buffer where the output mip will be stored
 * @param resOut    Resolution of the output mip
 * @param swizzleMask Vector indicating the index of each component of the 
 *      normal/detail map. x: normal X, y: normal Y, z: geometric roughness 
 *      or detail smoothness, w: detail AO
 */
static inline void CCAT(Pow2Mip_Box_Normal_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, const int2 resIn, PIX_TYPE_IN* imageOut, const int2 resOut, const ivec4s swizzleMask)
{
#pragma omp for
    for (int rowIdx = 0; rowIdx < resOut.y; rowIdx++)
    {
        int rowIn1Ptr = resIn.x * (2 * rowIdx);
        int rowIn2Ptr = rowIn1Ptr < ((resIn.y - 1) * resIn.x) ? rowIn1Ptr + resIn.x : rowIn1Ptr;
        int rowOutPtr = resOut.x * rowIdx;

        for (int u = 0; u < resOut.x; u++)
        {
            int xOffset = 2 * u;
            int xOffset2 = xOffset < (resIn.x - 1) ? xOffset + 1 : xOffset;
            PIX_TYPE_IN pixels[4] =
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

            //if (u == 0 && rowIdx == 0)
            //{
            //    for (int i = 0; i < 4; i++)
            //    {
            //        printf("Pixel %d: %f, %f, %f, %f\n", i, fpixels[i].x, fpixels[i].y, fpixels[i].z, fpixels[i].w);
            //    }
            //}

            vec4s normals[4];
            for (int i = 0; i < 4; i++)
            {
                float x = fpixels[i].raw[swizzleMask.x];
                float y = fpixels[i].raw[swizzleMask.y];
                x = 2.0f * x - 1.0f;
                y = 2.0f * y - 1.0f;
                normals[i] = (vec4s){ x, y, sqrtf(fmaxf(1.0f - (x * x) - (y * y), 0.0f)), 0 }; //HemiOctToVec(fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]);
            }

            //if (u == 0 && rowIdx == 0)
            //{
            //    for (int i = 0; i < 4; i++)
            //    {
            //        printf("Normal %d: %f, %f, %f\n", i, normals[i].x, normals[i].y, normals[i].z);
            //    }
            //}
            vec4s avgVec = { 0.25f, 0.25f, 0.25f, 0.25f };
            vec4 avgVecOut;
            glm_mat4_mulv(&(normals[0].raw), avgVec.raw, avgVecOut);
            vec3s avgNormal = glms_vec3_make(avgVecOut);
            //(vec3s){ 
            //    .x = 0.25 * normals[0].x + 0.25 * normals[1].x + 0.25 * normals[2].x + 0.25 * normals[3].x,
            //    .y = 0.25 * normals[0].y + 0.25 * normals[1].y + 0.25 * normals[2].y + 0.25 * normals[3].y,
            //    .z = 0.25 * normals[0].z + 0.25 * normals[1].z + 0.25 * normals[2].z + 0.25 * normals[3].z
            //};
            

            //if (u == 0 && rowIdx == 0)
            //{
            //    printf("Avg Normal: %f, %f, %f\n\n", avgNormal.x, avgNormal.y, avgNormal.z);
            //}
            avgNormal = glms_vec3_normalize(avgNormal);
            //vec2s avgHOct = VecToHemiOct(avgNormal.x, avgNormal.y);

            mat4s pixelMat = glms_mat4_transpose(glms_mat4_make((float*)fpixels));
            float avgZ = glms_vec4_dot(pixelMat.col[swizzleMask.z], avgVec);
            float avgW = glms_vec4_dot(pixelMat.col[swizzleMask.w], avgVec);
            vec4s outp;
            outp.raw[swizzleMask.x] = 0.5f * avgNormal.x + 0.5f;
            outp.raw[swizzleMask.y] = 0.5f * avgNormal.y + 0.5f;
            outp.raw[swizzleMask.z] = avgZ;
            outp.raw[swizzleMask.w] = avgW;

            imageOut[rowOutPtr + u] = FLOAT4_TO_PIX(imageOut, outp);
        }
    }
}

static inline void CCAT(ResolveGeoRoughness_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, half2* varianceBuffer, const int2 resIn, const ivec4s swizzleMask, const int hemiOct, const int detailMap, const float roughnessStrength)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for nowait
    for (long u = 0; u < pixelCount; u += 4)
    {
        PIX_TYPE_IN pixels[4];
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

        half2 hvariance[4];
        for (int i = 0; i < blockCount; i++)
        {
            hvariance[i] = varianceBuffer[u + i];
        }

        vec2s fvariance[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            fvariance[i] = Half2ToFloat2(hvariance[i]);
        }

        vec2s normals[4];

        float roughness[4];
        float mult = detailMap == 1 ? 0.5f : 1.0f;
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            float x = 2.0 * fpixels[i].raw[swizzleMask.x] - 1.0;
            float y = 2.0 * fpixels[i].raw[swizzleMask.y] - 1.0;
            float z = sqrt(1.0 - fminf(x * x + y * y, 1.0));
            normals[i] = hemiOct ? VecToHemiOct(x, y, z) : (vec2s){ fpixels[i].raw[swizzleMask.x], fpixels[i].raw[swizzleMask.y]};
            // variance of the normals on each axis. The values stored in varianceBuffer are the mip's averages of x^2 and y^2
            
            float varianceX = fvariance[i].x - fpixels[i].raw[swizzleMask.x] * fpixels[i].raw[swizzleMask.x];
            float varianceY = fvariance[i].y - fpixels[i].raw[swizzleMask.y] * fpixels[i].raw[swizzleMask.y];
            float normalRoughness = fminf(sqrt(varianceX * varianceX + varianceY * varianceY), 1.0f);
            float perceptualRougness = roughnessStrength * sqrtf(normalRoughness);
            // If this is a detail map, Z already contains overlay-blend perceptual smoothness information
            float detailSmoothness = fpixels[i].raw[swizzleMask.z];
           
            roughness[i] = detailMap ? fmaxf(0.0f, detailSmoothness - mult * perceptualRougness) : perceptualRougness;
        }

        vec4s outp[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            outp[i].raw[swizzleMask.x] = normals[i].x;
            outp[i].raw[swizzleMask.y] = normals[i].y;
            outp[i].raw[swizzleMask.z] = roughness[i];
            outp[i].raw[swizzleMask.w] = fpixels[i].raw[swizzleMask.w];
        }
#pragma omp simd
        for (int i = 0; i < blockCount; i++)
        {
            imageIn[u + i] = FLOAT4_TO_PIX(imageIn, outp[i]);
        }
    }
}

static inline void CCAT(ResolveHemiOctOnly_, PIX_TYPE_IN)(PIX_TYPE_IN* imageIn, const int2 resIn, const ivec4s swizzleMask)
{

    long pixelCount = (long)resIn.x * (long)resIn.y;
#pragma omp for nowait
    for (long u = 0; u < pixelCount; u += 4)
    {
        PIX_TYPE_IN pixels[4];
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

#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            float x = 2.0 * fpixels[i].raw[swizzleMask.x] - 1.0;
            float y = 2.0 * fpixels[i].raw[swizzleMask.y] - 1.0;
            float z = sqrt(1.0 - fminf(x * x + y * y, 1.0));
            normals[i] = VecToHemiOct(x, y, z);
        }

        vec4s outp[4];
#pragma omp simd
        for (int i = 0; i < 4; i++)
        {
            outp[i].raw[swizzleMask.x] = normals[i].x;
            outp[i].raw[swizzleMask.y] = normals[i].y;
            outp[i].raw[swizzleMask.z] = fpixels[i].raw[swizzleMask.z];
            outp[i].raw[swizzleMask.w] = fpixels[i].raw[swizzleMask.w];
        }
#pragma omp simd
        for (int i = 0; i < blockCount; i++)
        {
            imageIn[u + i] = FLOAT4_TO_PIX(imageIn, outp[i]);
        }
    }
}

void CCAT(GenMips_Pow2_Normal_,PIX_TYPE_IN)(
    PIX_TYPE_IN** mipChain, 
    half2** varianceMips, 
    const ivec4s swizzleMaskIn,
    const ivec4s swizzleMaskOut,
    const int2* mipResolutions, 
    int mipCount, 
    const int hemiOct,
    const float geoRoughnessStr, 
    const int detailMap
    )
{
    int threadCount = omp_get_num_procs();
    omp_set_num_threads(threadCount);
    //printf("Thread Count: %d\n", threadCount);
    //printf("Mip Count: %d\n", mipCount);
    #pragma omp parallel shared(mipChain, varianceMips, swizzleMaskIn, swizzleMaskOut, mipResolutions, mipCount,hemiOct, geoRoughnessStr, detailMap)
    {
        CCAT(InitNormalize_, PIX_TYPE_IN)(mipChain[0], mipResolutions[0], swizzleMaskIn, swizzleMaskOut, detailMap);

        for (int mIdx = 1; mIdx < mipCount; mIdx++)
        {
            CCAT(Pow2Mip_Box_Normal_, PIX_TYPE_IN)(mipChain[mIdx - 1], mipResolutions[mIdx - 1], mipChain[mIdx], mipResolutions[mIdx], swizzleMaskOut);
            // printf("Finished mip %d\n", mIdx);
        }

        if (geoRoughnessStr > 0.0f)
        {
            for (int mIdx = 0; mIdx < mipCount; mIdx++)
            {
                CCAT(ResolveGeoRoughness_, PIX_TYPE_IN)(mipChain[mIdx], varianceMips[mIdx], mipResolutions[mIdx], swizzleMaskOut, hemiOct, detailMap, geoRoughnessStr);
            }
        }
        else if (hemiOct)
        {
            for (int mIdx = 0; mIdx < mipCount; mIdx++)
            {
                CCAT(ResolveHemiOctOnly_, PIX_TYPE_IN)(mipChain[mIdx], mipResolutions[mIdx], swizzleMaskOut);
            }
        }
    }
}

void CCAT(GenVarianceMips_Pow2_, PIX_TYPE_IN)(
    PIX_TYPE_IN* mip0,
    half2** varianceMips,
    const ivec4s swizzleMaskIn,
    const int2* mipResolutions,
    int mipCount
    )
{
    int threadCount = omp_get_num_procs();
    omp_set_num_threads(threadCount);
    //printf("Thread Count: %d\n", threadCount);
    //printf("Mip Count: %d\n", mipCount);
#pragma omp parallel shared(mip0, varianceMips, swizzleMaskIn, mipResolutions, mipCount)
    {
        CCAT(InitVarianceDirect_, PIX_TYPE_IN)(mip0, varianceMips[0], mipResolutions[0], swizzleMaskIn);

        for (int mIdx = 1; mIdx < mipCount; mIdx++)
        {
            Pow2Mip_Box_Half2(varianceMips[mIdx - 1], mipResolutions[mIdx - 1], varianceMips[mIdx], mipResolutions[mIdx]);
            // printf("Finished mip %d\n", mIdx);
        }
    }
}



#undef CCAT2
#undef CCAT