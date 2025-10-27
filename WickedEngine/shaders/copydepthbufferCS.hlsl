#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#ifdef COPYDEPTH_MSAA
Texture2DMS<float> input_depth : register(t0);
#else
Texture2D<float> input_depth : register(t0);
#endif

RWTexture2D<float> output_depth_mip0 : register(u0);
RWTexture2D<float> output_depth_mip1 : register(u1);
RWTexture2D<float> output_depth_mip2 : register(u2);
RWTexture2D<float> output_depth_mip3 : register(u3);
RWTexture2D<float> output_depth_mip4 : register(u4);

RWTexture2D<float> output_lineardepth_mip0 : register(u5);
RWTexture2D<float> output_lineardepth_mip1 : register(u6);
RWTexture2D<float> output_lineardepth_mip2 : register(u7);
RWTexture2D<float> output_lineardepth_mip3 : register(u8);
RWTexture2D<float> output_lineardepth_mip4 : register(u9);

inline uint ComputeMipDimension(uint dimension, uint mip_level)
{
    dimension = max(dimension, 1u);
    return max(dimension >> mip_level, 1u);
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
    uint2 full_dim;
#ifdef COPYDEPTH_MSAA
    uint sample_count;
    input_depth.GetDimensions(full_dim.x, full_dim.y, sample_count);
#else
    input_depth.GetDimensions(full_dim.x, full_dim.y);
    const uint sample_count = 1;
#endif

    const uint2 pixel = DTid.xy;
    if (pixel.x >= full_dim.x || pixel.y >= full_dim.y)
    {
        return;
    }

    float depth = 0.0f;
#ifdef COPYDEPTH_MSAA
    [unroll]
    for (uint sample = 0; sample < sample_count; ++sample)
    {
        depth = max(depth, input_depth.Load(pixel, sample));
    }
#else
    depth = input_depth[pixel];
#endif

    depth = saturate(depth);

    float lineardepth = compute_lineardepth(depth) * GetCamera().z_far_rcp;

    output_depth_mip0[pixel] = depth;
    output_lineardepth_mip0[pixel] = lineardepth;

    if ((GTid.x & 1u) == 0u && (GTid.y & 1u) == 0u)
    {
        uint2 coord = pixel >> 1;
        uint2 dim = uint2(ComputeMipDimension(full_dim.x, 1), ComputeMipDimension(full_dim.y, 1));
        if (coord.x < dim.x && coord.y < dim.y)
        {
            output_depth_mip1[coord] = depth;
            output_lineardepth_mip1[coord] = lineardepth;
        }

        if ((GTid.x & 3u) == 0u && (GTid.y & 3u) == 0u)
        {
            coord = pixel >> 2;
            dim = uint2(ComputeMipDimension(full_dim.x, 2), ComputeMipDimension(full_dim.y, 2));
            if (coord.x < dim.x && coord.y < dim.y)
            {
                output_depth_mip2[coord] = depth;
                output_lineardepth_mip2[coord] = lineardepth;
            }

            if ((GTid.x & 7u) == 0u && (GTid.y & 7u) == 0u)
            {
                coord = pixel >> 3;
                dim = uint2(ComputeMipDimension(full_dim.x, 3), ComputeMipDimension(full_dim.y, 3));
                if (coord.x < dim.x && coord.y < dim.y)
                {
                    output_depth_mip3[coord] = depth;
                    output_lineardepth_mip3[coord] = lineardepth;
                }

                if ((GTid.x & 15u) == 0u && (GTid.y & 15u) == 0u)
                {
                    coord = pixel >> 4;
                    dim = uint2(ComputeMipDimension(full_dim.x, 4), ComputeMipDimension(full_dim.y, 4));
                    if (coord.x < dim.x && coord.y < dim.y)
                    {
                        output_depth_mip4[coord] = depth;
                        output_lineardepth_mip4[coord] = lineardepth;
                    }
                }
            }
        }
    }
}
