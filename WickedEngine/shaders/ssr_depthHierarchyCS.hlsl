#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float2> input : register(t0);

RWTexture2D<float2> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (all(DTid.xy < postprocess.params0.xy))
	{
		if (postprocess.params0.z == 1)
		{
			uint2 dim;
			texture_depth.GetDimensions(dim.x, dim.y);

			float2 uv = (DTid.xy + 0.5) / dim * ssr_downscalefactor;

			float4 depths = texture_depth.GatherRed(sampler_point_clamp, uv);

			float depthMax = max(max(depths.x, depths.y), max(depths.z, depths.w));
			float depthMin = min(min(depths.x, depths.y), min(depths.z, depths.w));

			output[DTid.xy] = float2(depthMax, depthMin);
		}
		else
		{
			float2 uv = (DTid.xy + 0.5) / postprocess.params0.xy;

			float4 depthsRed = input.GatherRed(sampler_point_clamp, uv);
			float4 depthsGreen = input.GatherGreen(sampler_point_clamp, uv);

			float depthMax = max(max(depthsRed.x, depthsRed.y), max(depthsRed.z, depthsRed.w));
			float depthMin = min(min(depthsGreen.x, depthsGreen.y), min(depthsGreen.z, depthsGreen.w));

			output[DTid.xy] = float2(depthMax, depthMin);
		}
	}
}
