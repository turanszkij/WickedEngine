#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Purpose of this shader: blends the two sides of edges together by uv-mirroring

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<half4> input : register(t0);
Texture2D<half2> mask : register(t1);
Texture2D<uint2> edgemap : register(t2);

float4 main(float4 pos : SV_Position) : SV_Target
{
	const uint2 pixel = pos.xy;
	const uint2 edge = edgemap[pixel];
	if (all(edge == 0))
		return 0;

	ShaderCamera camera = GetCamera();
	const float3 P = reconstruct_position(pos.xy * postprocess.resolution_rcp, texture_depth[pixel], camera.inverse_view_projection);
	
	const half current_id = mask[pixel].x;
	const half edge_id = mask[edge].x;
	const half distance_falloff = max(mask[pixel].y, mask[edge].y) * postprocess.params0.y;
	
	const half2 best_offset = (float2)edge - (float2)pixel;
	const half best_dist = length(best_offset);
	
	const float2 uv = (pixel + best_offset * 2 + 0.5) * postprocess.resolution_rcp; // *2 : mirroring to the other side of the seam
	const half4 rrrr = input.GatherRed(sampler_linear_clamp, uv);
	const half4 gggg = input.GatherGreen(sampler_linear_clamp, uv);
	const half4 bbbb = input.GatherBlue(sampler_linear_clamp, uv);
	const half4 iiii = mask.GatherRed(sampler_linear_clamp, uv);
	const half4 dddd = texture_depth.GatherRed(sampler_linear_clamp, uv);
	half3 other_color = 0;
	half sum = 0;
	half dweight = 0;
	for (uint i = 0; i < 4; ++i)
	{
		// The gathered values are validated and invalid samples discarded
		const half id = iiii[i];
		if (id != current_id && id == edge_id) // only allow blending into different region, and only if it's the same as edge id
		{
			const float3 p0 = reconstruct_position(uv, dddd[i], camera.inverse_view_projection);
			const half diff = length(p0 - P);
			dweight += saturate(1 - diff / distance_falloff); // distance-based weighting
			other_color += half3(rrrr[i], gggg[i], bbbb[i]);
			sum++;
		}
	}
	
	float4 color = 0;
	if (sum > 0)
	{
		other_color /= sum;
		dweight /= sum;
		const half weight = saturate(0.5 - best_dist / postprocess.params0.x) * dweight;
		color = float4(lerp(input[pixel].rgb, other_color.rgb, weight), 1);
		//color = float4(weight, 0, 0, 1);
	}
	return color;
}
