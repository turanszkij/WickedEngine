#include "deferredLightHF.hlsli"
#include "fogHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderEntityType light = EntityArray[(uint)g_xColor.x];

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float depth = max(input.pos.z, texture_depth.SampleLevel(sampler_linear_clamp, ScreenCoord, 0));
	float3 P = getPosition(ScreenCoord, depth);
	float3 V = g_xCamera_CamPos - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	float marchedDistance = 0;
	float accumulation = 0;

	float3 L = light.positionWS - P;
	float dist = length(L);
	L /= dist;

	// reposition ray start to be inside light all times:
	dist = min(dist, light.range);
	P = light.positionWS - L * dist;

	// Perform ray marching to integrate light volume along view ray:
	float sum = 0;
	while (dist <= light.range && marchedDistance < cameraDistance)
	{
		float att = (light.energy * (light.range / (light.range + 1 + dist)));
		float attenuation = (att * (light.range - dist) / light.range);

		[branch]
		if (light.additionalData_index >= 0) {
			attenuation *= texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.additionalData_index), 1 - dist / light.range * (1 - light.shadowBias)).r;
		}

		attenuation *= GetFog(cameraDistance - marchedDistance);

		accumulation += attenuation;

		const float stepSize = 0.1f;
		marchedDistance += stepSize;
		P = P + V * stepSize;
		L = light.positionWS - P;
		dist = length(L);
		L /= dist;

		sum += 1.0f;
	}

	accumulation /= sum;

	return max(0, float4(accumulation.xxx * light.GetColor().rgb * light.energy, 1));
}