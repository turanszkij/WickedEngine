#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float> input : register(t0);

RWTexture2D<float> output_lineardepth_mip0 : register(u0);
RWTexture2D<float> output_lineardepth_mip1 : register(u1);
RWTexture2D<float> output_lineardepth_mip2 : register(u2);
RWTexture2D<float> output_lineardepth_mip3 : register(u3);
RWTexture2D<float> output_lineardepth_mip4 : register(u4);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	const uint2 pixel = DTid.xy;
	const float depth = input[pixel];
	float lineardepth = compute_lineardepth(depth) * GetCamera().z_far_rcp;
	output_lineardepth_mip0[pixel] = lineardepth;
	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		output_lineardepth_mip1[pixel / 2] = lineardepth;
	}
	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		output_lineardepth_mip2[pixel / 4] = lineardepth;
	}
	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		output_lineardepth_mip3[pixel / 8] = lineardepth;
	}
	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		output_lineardepth_mip4[pixel / 16] = lineardepth;
	}
}
