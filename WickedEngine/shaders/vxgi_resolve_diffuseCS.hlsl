#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "voxelConeTracingHF.hlsli"
#include "lightingHF.hlsli"

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
	
	const float3 N = decode_oct(texture_normal.SampleLevel(sampler_point_clamp, uv, 0));
	const float3 P = reconstruct_position(uv, depth);

	Texture3D<float4> voxels = bindless_textures3D[GetFrame().vxgi.texture_radiance];
	float4 trace = ConeTraceDiffuse(voxels, P, N);
	float4 color = float4(trace.rgb, 1);
	color.rgb += GetAmbient(N) * (1 - trace.a);
	
	output[pixel] = color;

}
