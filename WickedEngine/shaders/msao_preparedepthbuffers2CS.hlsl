// This is a modified version of the SSAO from Microsoft MiniEngine at https://github.com/Microsoft/DirectX-Graphics-Samples

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(DS4x, float, TEXSLOT_ONDEMAND0);
RWTEXTURE2D(DS8x, float, 0);
RWTEXTURE2DARRAY(DS8xAtlas, float, 1);
RWTEXTURE2D(DS16x, float, 2);
RWTEXTURE2DARRAY(DS16xAtlas, float, 3);

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float m1 = DS4x[DTid.xy << 1];

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
