#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Purpose of this shader: blends the two sides of edges together by uv-mirroring

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<half4> input : register(t0);
Texture2D<half> mask : register(t1);
Texture2D<uint2> edgemap : register(t2);

RWTexture2D<half4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 edge = edgemap[DTid.xy];
	if (all(edge == 0))
		return;

	ShaderCamera camera = GetCamera();
	const float3 P = reconstruct_position(float2(DTid.xy + 0.5) * postprocess.resolution_rcp, texture_depth[DTid.xy], camera.inverse_view_projection);
	
	const half current_id = mask[DTid.xy];
	
	half2 best_offset = (float2)edge - (float2)DTid.xy;
	half best_dist = length(best_offset);
	
	const float2 uv = (DTid.xy + best_offset * 2 + 0.5) * postprocess.resolution_rcp; // *2 : mirroring to the other side of the seam
	const half4 rrrr = input.GatherRed(sampler_linear_clamp, uv);
	const half4 gggg = input.GatherGreen(sampler_linear_clamp, uv);
	const half4 bbbb = input.GatherBlue(sampler_linear_clamp, uv);
	const half4 iiii = mask.GatherRed(sampler_linear_clamp, uv);
	const half4 dddd = texture_depth.GatherRed(sampler_linear_clamp, uv);
	half3 other_color = 0;
	half sum = 0;
	float dweight = 0;
	for (uint i = 0; i < 4; ++i)
	{
		// The gathered values are validated and invalid samples discarded
		const half id = iiii[i];
		if (id != current_id)
		{
			const float3 p0 = reconstruct_position(uv, dddd[i], camera.inverse_view_projection);
			const float diff = length(p0 - P);
			dweight += saturate(1 - diff / postprocess.params0.y); // distance-based weighting
			other_color += half3(rrrr[i], gggg[i], bbbb[i]);
			sum++;
		}
	}
	if (sum > 0)
	{
		other_color /= sum;
		dweight /= sum;
		float weight = saturate(0.5 - best_dist / postprocess.params0.x) * dweight;
		output[DTid.xy] = float4(lerp(output[DTid.xy].rgb, other_color.rgb, weight), 1);
		//output[DTid.xy] = float4(weight, 0, 0, 1);
	}
}
