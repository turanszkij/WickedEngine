#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK // fix the lack of depth testing
#include "volumetricLightHF.hlsli"
#include "fogHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderEntity light = load_entity(GetFrame().lightarray_offset + (uint)g_xColor.x);

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float depth = max(input.pos.z, texture_depth.SampleLevel(sampler_point_clamp, ScreenCoord, 0));
	float3 P = reconstruct_position(ScreenCoord, depth);
	float3 V = GetCamera().position - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	float marchedDistance = 0;
	float3 accumulation = 0;

	float3 rayEnd = GetCamera().position;
	// todo: rayEnd should be clamped to the closest cone intersection point when camera is outside volume
	
	const uint sampleCount = 16;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// dither ray start to help with undersampling:
	P = P + V * stepSize * dither(input.pos.xy);

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for (uint i = 0; i < sampleCount; ++i)
	{
		float3 L = light.position - P;
		const float dist2 = dot(L, L);
		const float dist = sqrt(dist2);
		L /= dist;

		const float spot_factor = dot(L, light.GetDirection());
		const float spot_cutoff = light.GetConeAngleCos();

		[branch]
		if (spot_factor > spot_cutoff)
		{
			const float range = light.GetRange();
			const float range2 = range * range;
			float3 attenuation = attenuation_spotlight(dist, dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());

			[branch]
			if (light.IsCastingShadow())
			{
				float4 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(P, 1));
				shadow_pos.xyz /= shadow_pos.w;
				float2 shadow_uv = shadow_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
				[branch]
				if ((saturate(shadow_uv.x) == shadow_uv.x) && (saturate(shadow_uv.y) == shadow_uv.y))
				{
					attenuation *= shadow_2D(light, shadow_pos.xyz, shadow_uv.xy, 0);
				}
			}

			// Evaluate sample height for exponential fog calculation, given 0 for V:
			attenuation *= GetFogAmount(cameraDistance - marchedDistance, P, float3(0.0, 0.0, 0.0));
			attenuation *= ComputeScattering(saturate(dot(L, -V)));

			accumulation += attenuation;
		}

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation * light.GetColor().rgb, 1));
}
