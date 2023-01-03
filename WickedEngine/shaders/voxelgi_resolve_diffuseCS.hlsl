#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "voxelConeTracingHF.hlsli"
#include "lightingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input_prev : register(t0);

RWTexture2D<float4> output : register(u0);

// If this is enabled, disocclusion regions will be retraced, otherwise they will be filled by primary sample color:
#define HIGH_QUALITY_DISOCCLUSION
static const float disocclusion_threshold = 1;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel_base = DTid.xy * VXGI_DIFFUSE_UPSAMPLING;
	const uint2 pixel_offset = unflatten2D(GetFrame().frame_count % (VXGI_DIFFUSE_UPSAMPLING * VXGI_DIFFUSE_UPSAMPLING), VXGI_DIFFUSE_UPSAMPLING);
	const uint2 pixel = clamp(pixel_base + pixel_offset, 0, postprocess.resolution - 1);
	const float2 uv = ((float2)pixel + 0.5) * postprocess.resolution_rcp;

	const float depth = texture_depth[pixel];
	const float roughness = texture_roughness[pixel];

	const float3 N = decode_oct(texture_normal[pixel]);
	const float3 P = reconstruct_position(uv, depth);

	Texture3D<float4> voxels = bindless_textures3D[GetFrame().vxgi.texture_radiance];

	float4 trace = ConeTraceDiffuse(voxels, P, N);
	float4 color = float4(trace.rgb, 1);
	color.rgb += GetAmbient(N) * (1 - trace.a);

	output[pixel] = color;

	for (uint x = 0; x < VXGI_DIFFUSE_UPSAMPLING; ++x)
	{
		for (uint y = 0; y < VXGI_DIFFUSE_UPSAMPLING; ++y)
		{
			const uint2 neighbor_offset = uint2(x, y);
			const uint2 pixel_neighbor = clamp(pixel_base + neighbor_offset, 0, postprocess.resolution - 1);
			if (any(neighbor_offset != pixel_offset))
			{
				const float2 velocity = texture_velocity[pixel_neighbor];
				const float2 uv_neighbor = (pixel_neighbor + 0.5) * postprocess.resolution_rcp;
				const float2 uv_neighbor_prev = uv_neighbor + velocity;
				bool reprojection_valid = is_saturated(uv_neighbor_prev);

				const float depth_neighbor = texture_depth[pixel_neighbor];
				if (reprojection_valid)
				{
					const float depth_neighbor_prev = texture_depth_history.SampleLevel(sampler_linear_clamp, uv_neighbor_prev, 0);
					const float lineardepth_neighbor = compute_lineardepth(depth_neighbor);
					const float lineardepth_neighbor_prev = compute_lineardepth(depth_neighbor_prev);
					reprojection_valid = reprojection_valid && abs(lineardepth_neighbor - lineardepth_neighbor_prev) < disocclusion_threshold;
				}

				if (reprojection_valid)
				{
					output[pixel_neighbor] = input_prev.SampleLevel(sampler_linear_clamp, uv_neighbor_prev, 0);
				}
				else
				{
#ifdef HIGH_QUALITY_DISOCCLUSION
					const float3 N = decode_oct(texture_normal[pixel_neighbor]);
					const float3 P = reconstruct_position(uv_neighbor, depth_neighbor);
					float4 trace = ConeTraceDiffuse(voxels, P, N);
					float4 color = float4(trace.rgb, 1);
					color.rgb += GetAmbient(N) * (1 - trace.a);
					output[pixel_neighbor] = color;
#else
					output[pixel_neighbor] = color;
					//output[pixel_neighbor] = float4(1,0,0,1);
#endif // HIGH_QUALITY_DISOCCLUSION
				}

			}
		}
	}
}
