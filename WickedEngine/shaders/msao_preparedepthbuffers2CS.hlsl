// This is a modified version of the SSAO from Microsoft MiniEngine at https://github.com/Microsoft/DirectX-Graphics-Samples

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float> DS4x : register(t0);

RWTexture2D<float> DS8x : register(u0);
RWTexture2DArray<float> DS8xAtlas : register(u1);
RWTexture2D<float> DS16x : register(u2);
RWTexture2DArray<float> DS16xAtlas : register(u3);

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	DS4x.GetDimensions(dim.x, dim.y);

    float m1 = DS4x[min(DTid.xy << 1, dim - 1)];

    uint2 st = DTid.xy;
    uint2 stAtlas = st >> 2;
    uint stSlice = (st.x & 3) | ((st.y & 3) << 2);
    DS8x[st] = m1;
    DS8xAtlas[uint3(stAtlas, stSlice)] = m1;

    if ((GI & 011) == 0)
    {
        uint2 st = DTid.xy >> 1;
        uint2 stAtlas = st >> 2;
        uint stSlice = (st.x & 3) | ((st.y & 3) << 2);
        DS16x[st] = m1;
        DS16xAtlas[uint3(stAtlas, stSlice)] = m1;
    }
}
