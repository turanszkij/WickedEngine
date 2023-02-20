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
	if (length(rayEnd - light.position) > light.GetRange())
	{
		// if we are outside the light volume, then rayEnd will be the traced sphere frontface:
		float t = trace_sphere(rayEnd, -V, light.position, light.GetRange());
		rayEnd = rayEnd - t * V;
	}

	const uint sampleCount = 16;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// dither ray start to help with undersampling:
	P = P + V * stepSize * dither(input.pos.xy);

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for(uint i = 0; i < sampleCount; ++i)
	{
		float3 L = light.position - P;
		const float3 Lunnormalized = L;
		const float dist2 = dot(L, L);
		const float dist = sqrt(dist2);
		L /= dist;

		const float range = light.GetRange();
		const float range2 = range * range;
		float3 attenuation = attenuation_pointlight(dist, dist2, range, range2);

		[branch]
		if (light.IsCastingShadow())
		{
			attenuation *= shadow_cube(light, Lunnormalized);
		}

		// Evaluate sample height for height fog calculation, given 0 for V:
		attenuation *= GetFogAmount(cameraDistance - marchedDistance, P, float3(0.0, 0.0, 0.0));
		attenuation *= ComputeScattering(saturate(dot(L, -V)));
		
		accumulation += attenuation;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation * light.GetColor().rgb, 1));
}
