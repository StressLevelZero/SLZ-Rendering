
__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void transpose (
    __r
, float a)
{
    const int2 pos = {get_global_id (0), get_global_id (1)};

    y [i] += a * x [i];
}
