// This is a modified version of the SSAO from Microsoft MiniEngine at https://github.com/Microsoft/DirectX-Graphics-Samples

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTexture2D<float> DS2x : register(u0);
RWTexture2DArray<float> DS2xAtlas : register(u1);
RWTexture2D<float> DS4x : register(u2);
RWTexture2DArray<float> DS4xAtlas : register(u3);

groupshared float g_CacheW[256];

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	texture_lineardepth.GetDimensions(dim.x, dim.y);

    uint2 startST = Gid.xy << 4 | GTid.xy;
    uint destIdx = GTid.y << 4 | GTid.x;
    g_CacheW[destIdx + 0] = texture_lineardepth[min(startST | uint2(0, 0), dim - 1)];
    g_CacheW[destIdx + 8] = texture_lineardepth[min(startST | uint2(8, 0), dim - 1)];
    g_CacheW[destIdx + 128] = texture_lineardepth[min(startST | uint2(0, 8), dim - 1)];
    g_CacheW[destIdx + 136] = texture_lineardepth[min(startST | uint2(8, 8), dim - 1)];

    GroupMemoryBarrierWithGroupSync();

    uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

    float w1 = g_CacheW[ldsIndex];

    uint2 st = DTid.xy;
    uint slice = (st.x & 3) | ((st.y & 3) << 2);
    DS2x[st] = w1;
    DS2xAtlas[uint3(st >> 2, slice)] = w1;

    if ((GI & 011) == 0)
    {
        st = DTid.xy >> 1;
        slice = (st.x & 3) | ((st.y & 3) << 2);
        DS4x[st] = w1;
        DS4xAtlas[uint3(st >> 2, slice)] = w1;
    }

}
