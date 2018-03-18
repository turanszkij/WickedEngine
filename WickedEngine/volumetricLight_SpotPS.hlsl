#define DISABLE_TRANSPARENT_SHADOWMAP
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
	float3 accumulation = 0;

	float3 L = light.positionWS - P;
	float dist = length(L);
	L /= dist;

	float3 rayEnd = g_xCamera_CamPos;
	// todo: when rayEnd (camera) is not inside volume, then the rayEnd should be the traced cone position instead!

	const uint sampleCount = 128;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// Perform ray marching to integrate light volume along view ray:
	for (uint i = 0; i < sampleCount; ++i)
	{
		marchedDistance += stepSize;
		P = P + V * stepSize;
		float3 L = light.positionWS - P;
		float dist = length(L);
		L /= dist;

		float SpotFactor = dot(L, light.directionWS);
		float spotCutOff = light.coneAngleCos;

		[branch]
		if (SpotFactor > spotCutOff)
		{
			float att = (light.energy * (light.range / (light.range + 1 + dist)));
			float3 attenuation = (att * (light.range - dist) / light.range);
			attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

			[branch]
			if (light.additionalData_index >= 0)
			{
				float4 ShPos = mul(float4(P, 1), MatrixArray[light.additionalData_index + 0]);
				ShPos.xyz /= ShPos.w;
				float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
				[branch]
				if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
				{
					attenuation *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.additionalData_index);
				}
			}

			attenuation *= GetFog(distance(P, g_xCamera_CamPos));

			accumulation += attenuation;
		}

	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation * light.GetColor().rgb * light.energy, 1));
}