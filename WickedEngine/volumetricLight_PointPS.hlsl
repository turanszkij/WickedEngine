#define DISABLE_TRANSPARENT_SHADOWMAP
#include "volumetricLightHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderEntity light = EntityArray[(uint)g_xColor.x];

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float depth = max(input.pos.z, texture_depth.SampleLevel(sampler_linear_clamp, ScreenCoord, 0));
	float3 P = reconstructPosition(ScreenCoord, depth);
	float3 V = g_xCamera_CamPos - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	float marchedDistance = 0;
	float accumulation = 0;

	float3 rayEnd = g_xCamera_CamPos;
	if (length(rayEnd - light.positionWS) > light.range)
	{
		// if we are outside the light volume, then rayEnd will be the traced sphere frontface:
		float t = Trace_sphere(rayEnd, -V, light.positionWS, light.range);
		rayEnd = rayEnd - t * V;
	}

	const uint sampleCount = 128;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for(uint i = 0; i < sampleCount; ++i)
	{
		float3 L = light.positionWS - P;
		const float3 Lunnormalized = L;
		const float dist2 = dot(L, L);
		const float dist = sqrt(dist2);
		L /= dist;

		const float range2 = light.range * light.range;
		const float att = saturate(1.0 - (dist2 / range2));
		float attenuation = att * att;

		[branch]
		if (light.IsCastingShadow()) {
			attenuation *= shadowCube(light, Lunnormalized);
		}

		attenuation *= GetFogAmount(cameraDistance - marchedDistance);

		accumulation += attenuation;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation.xxx * light.GetColor().rgb * light.energy, 1));
}