#define CCAT2(x,y) x##y
#define CCAT(x,y) CCAT2(x,y)

#define CCATN2(x,y,z) x##y##_##z
#define CCATN(x,y,z) CCATN2(x,y,z)
#define xstr(s) str(s)
#define str(s) #s

#ifndef PIX_TYPE_IN
#error Template type PIX_TYPE_IN not defined
#endif

/** @brief Complete processing of a normal or detail map image. Can generate mips, re-encode
 * normal directions to hemi-oct, and/or generate geometric roughness information.
 *
 * @param formatOut Pixel format to convert the output to. Restricted to
 *      different vector lengths of the same component format as the input
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
 * @return Error code indicating if anything failed
 */
TXPErrorCode CCAT(ProcessNormalMap_, PIX_TYPE_IN)(TxpTextureFormat formatOut, void* image, int2 imageRes, TxpTex2D* output,
    ivec4s swizzleMaskIn, ivec4s swizzleMaskOut,
    int yFlip, int detailMap, int hemiOct, float geoRoughnessStr, int genMips)
{
    TXPErrorCode err = TXP_RETURN_SUCCESS;
    //DebugLog(UNITY_LOG_LEVEL_LOG, "Chose type conversion %s\n", xstr(CCATN(ProcessNormalMap_, PIX_TYPE_IN, PIX_TYPE_4)));
    switch (formatOut)
    {
    case (PIX_TYPE_1_ENUM + 1): err = CCATN(ProcessNormalMap_, PIX_TYPE_IN, PIX_TYPE_2)(image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips); break;
    case (PIX_TYPE_1_ENUM + 2): err = CCATN(ProcessNormalMap_, PIX_TYPE_IN, PIX_TYPE_3)(image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips); break;
    case (PIX_TYPE_1_ENUM + 3): err = CCATN(ProcessNormalMap_, PIX_TYPE_IN, PIX_TYPE_4)(image, imageRes, output, swizzleMaskIn, swizzleMaskOut, yFlip, detailMap, hemiOct, geoRoughnessStr, genMips); break;
    default: DebugLog(UNITY_LOG_LEVEL_ERROR, "SLZ TexProc: Invalid type conversion, can't convert between texture pixel types %d and %d\n", formatOut, PIX_TYPE_1_ENUM); err = TXP_RETURN_INVALID_ARGS; break;
    }
    return err;
}

#undef CCAT2
#undef CCAT
#undef CCATN2
#undef CCATN
#undef xstr
#undef str