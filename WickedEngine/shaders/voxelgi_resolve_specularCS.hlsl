#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "voxelConeTracingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input_prev : register(t0);

RWTexture2D<float4> output : register(u0);

// If this is enabled, disocclusion regions will be retraced, otherwise they will be filled by primary sample color:
//#define HIGH_QUALITY_DISOCCLUSION
static const float disocclusion_threshold = 0.5;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel_base = DTid.xy * 2;
	const uint2 pixel_offset = unflatten2D(GetFrame().frame_count % 4, 2);
	const uint2 pixel = pixel_base + pixel_offset;
	const float2 uv = ((float2)pixel + 0.5) * postprocess.resolution_rcp;

	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	const float roughness = texture_roughness[pixel];

	const float3 N = decode_oct(texture_normal[pixel]);
	const float3 P = reconstruct_position(uv, depth);
	const float3 V = normalize(GetCamera().position - P);

	Texture3D<float4> voxels = bindless_textures3D[GetFrame().texture_voxelgi_index];
	float4 color = ConeTraceSpecular(voxels, P, N, V, roughness * roughness, pixel.xy);

	output[pixel] = color;

	for (uint x = 0; x < 2; ++x)
	{
		for (uint y = 0; y < 2; ++y)
		{
			const uint2 neighbor_offset = uint2(x, y);
			const uint2 pixel_neighbor = pixel_base + neighbor_offset;
			if (any(neighbor_offset != pixel_offset))
			{
#if 0
				output[pixel_neighbor] = input_prev[pixel_neighbor];
#else
				const float2 velocity = texture_velocity[pixel_neighbor];
				const float2 uv_neighbor = (pixel_neighbor + 0.5) * postprocess.resolution_rcp;
				const float2 uv_neighbor_prev = uv_neighbor + velocity;

				const float depth_neighbor = texture_depth[pixel_neighbor];
				const float depth_neighbor_prev = texture_depth_history.SampleLevel(sampler_linear_clamp, uv_neighbor_prev, 0);
				const float lineardepth_neighbor = compute_lineardepth(depth_neighbor);
				const float lineardepth_neighbor_prev = compute_lineardepth(depth_neighbor_prev);
				if (abs(lineardepth_neighbor - lineardepth_neighbor_prev) > disocclusion_threshold)
				{
					// invalid reprojection
#ifdef HIGH_QUALITY_DISOCCLUSION
					const float roughness = texture_roughness[pixel_neighbor];
					const float3 N = decode_oct(texture_normal[pixel_neighbor]);
					const float3 P = reconstruct_position(uv_neighbor, depth_neighbor);
					const float3 V = normalize(GetCamera().position - P);
					float4 color = ConeTraceSpecular(voxels, P, N, V, roughness * roughness, pixel.xy);
					output[pixel_neighbor] = color;
#else
					output[pixel_neighbor] = color;
					//output[pixel_neighbor] = float4(1,0,0,1);
#endif // HIGH_QUALITY_DISOCCLUSION
				}
				else
				{
					// valid reprojection
					output[pixel_neighbor] = input_prev.SampleLevel(sampler_linear_clamp, uv_neighbor_prev, 0);
				}
#endif
			}
		}
	}
}
