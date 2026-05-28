#if !defined(SLZ_FGD_INCLUDED)
#define SLZ_FGD_INCLUDED

TEXTURE2D(_FgdGgx);

half2 SampleFdg(TEXTURE2D(fgd), const half NoV, const half perceptualR)
{
    uint2 dim = 0;
    uint lvl = 0;
    fgd.GetDimensions(0, dim.x, dim.y, lvl);
    half2 coords = Remap01ToHalfTexelCoord(half2(sqrt(NoV), perceptualR), dim);
    return SAMPLE_TEXTURE2D_LOD(fgd, sampler_LinearClamp, coords, 0).rg;
}

#endif