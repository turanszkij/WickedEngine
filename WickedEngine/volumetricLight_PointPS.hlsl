#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK // fix the lack of depth testing
#include "volumetricLightHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + (uint)g_xColor.x];

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float depth = max(input.pos.z, texture_depth.SampleLevel(sampler_point_clamp, ScreenCoord, 2));
	float3 P = reconstructPosition(ScreenCoord, depth);
	float3 V = g_xCamera_CamPos - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	float marchedDistance = 0;
	float3 accumulation = 0;

	float3 rayEnd = g_xCamera_CamPos;
	if (length(rayEnd - light.position) > light.GetRange())
	{
		// if we are outside the light volume, then rayEnd will be the traced sphere frontface:
		float t = Trace_sphere(rayEnd, -V, light.position, light.GetRange());
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

		const float range2 = light.GetRange() * light.GetRange();
		const float att = saturate(1.0 - (dist2 / range2));
		float3 attenuation = att * att;

		[branch]
		if (light.IsCastingShadow()) {
			attenuation *= shadowCube(light, L, Lunnormalized);
		}

		attenuation *= GetFogAmount(cameraDistance - marchedDistance);

		accumulation += attenuation;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation * light.GetColor().rgb * light.GetEnergy(), 1));
}
