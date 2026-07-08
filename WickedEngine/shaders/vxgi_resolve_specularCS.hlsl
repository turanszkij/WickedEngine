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

	const half3 normal_roughness = texture_normal_roughness.SampleLevel(sampler_point_clamp, uv, 0).rgb;
	const float roughness = normal_roughness.b;

	// Early-out for rough surfaces: the reflection is negligible and already
	// covered by the diffuse GI / environment probes. Writing zero (including
	// alpha) leaves the existing indirect specular untouched in the shading
	// blend (vxgi_specular.rgb * F + indirect.specular * (1 -
	// vxgi_specular.a)).
	if (roughness > VXGI_SPECULAR_MAX_ROUGHNESS)
	{
		output[pixel] = 0;
		return;
	}

	const float3 N = decode_normal(normal_roughness.rg);
	const float3 P = reconstruct_position(uv, depth);
	const float3 V = normalize(GetCamera().frustum_corners.screen_to_nearplane(uv) - P); // ortho support

	Texture3D<half4> voxels = bindless_textures3D_half4[descriptor_index(GetFrame().vxgi.texture_radiance)];
	half4 color = ConeTraceSpecular(voxels, P, N, V, roughness * roughness, pixel);
	output[pixel] = color;
}
