#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

// Define this to use reduced precision, but faster depth buffer:
#define USE_LINEARDEPTH

RWTEXTURE2D(output, unorm float, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float range = xPPParams0.x;
	const uint sampleCount = xPPParams0.y;

	float seed = 1;
	const float3 noise = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;
	
	// reconstruct flat normals from depth buffer:
	uint2 dim;
	texture_depth.GetDimensions(dim.x, dim.y);
	const float2 dim_rcp = rcp(dim);

	const float2 uv1 = uv + float2(1, 0) * dim_rcp;
	const float2 uv2 = uv + float2(0, -1) * dim_rcp;

	const float depth0 = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	const float depth1 = texture_depth.SampleLevel(sampler_linear_clamp, uv1, 0);
	const float depth2 = texture_depth.SampleLevel(sampler_linear_clamp, uv2, 0);

	const float3 p0 = reconstructPosition(uv, depth0);
	const float3 p1 = reconstructPosition(uv1, depth1);
	const float3 p2 = reconstructPosition(uv2, depth2);

	const float3 normal = normalize(cross(p2 - p0, p1 - p0));

	const float3 P = p0;

	const float3 tangent = normalize(noise - normal * dot(noise, normal));
	const float3 bitangent = cross(normal, tangent);
	const float3x3 tangentSpace = float3x3(tangent, bitangent, normal);

	float ao = 0;
	for (uint i = 0; i < sampleCount; ++i)
	{
		const float2 hamm = hammersley2d(i, sampleCount);
		const float3 hemisphere = hemispherepoint_uniform(hamm.x, hamm.y);
		const float3 cone = mul(hemisphere, tangentSpace);
		const float ray_range = range * lerp(0.2f, 1.0f, rand(seed, uv)); // modulate ray-length a bit to avoid uniform look
		const float3 sam = P + cone * ray_range;

		float4 vProjectedCoord = mul(g_xFrame_MainCamera_VP, float4(sam, 1.0f));
		vProjectedCoord.xyz /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		if (is_saturated(vProjectedCoord.xy))
		{
#ifdef USE_LINEARDEPTH
			const float ray_depth_real = vProjectedCoord.w; // .w is also linear depth, could be also written as getLinearDepth(vProjectedCoord.z)
			const float ray_depth_sample = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0) * g_xFrame_MainCamera_ZFarP;
			const float depth_fix = 1 - saturate(abs(ray_depth_real - ray_depth_sample) * 0.2f); // too much depth difference cancels the effect
			ao += (ray_depth_sample < ray_depth_real) * depth_fix;
#else
			const float ray_depth_real = vProjectedCoord.z;
			const float ray_depth_sample = texture_depth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
			const float depth_fix = 1 - saturate(abs(vProjectedCoord.w - getLinearDepth(ray_depth_sample)) * 0.2f); // too much depth difference cancels the effect (vProjectedCoord.w is also linear depth)
			ao += (ray_depth_sample > ray_depth_real) * depth_fix;
#endif // USE_LINEARDEPTH
		}
	}
	ao /= (float)sampleCount;

	output[DTid.xy] = saturate(1 - ao);
}
