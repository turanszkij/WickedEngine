#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "voxelConeTracingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel = DTid.xy;
	const float2 uv = ((float2)pixel + 0.5) * postprocess.resolution_rcp;

	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
	if (depth == 0)
		return;

	const float roughness = texture_roughness.SampleLevel(sampler_point_clamp, uv, 0);
	const float3 N = decode_oct(texture_normal.SampleLevel(sampler_point_clamp, uv, 0));
	const float3 P = reconstruct_position(uv, depth);
	const float3 V = normalize(GetCamera().position - P);

	Texture3D<float4> voxels = bindless_textures3D[GetFrame().vxgi.texture_radiance];
	float4 color = ConeTraceSpecular(voxels, P, N, V, roughness * roughness, pixel);
	output[pixel] = color;
}
