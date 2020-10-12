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
	float3 accumulation = 0;

	float3 rayEnd = g_xCamera_CamPos;
	// todo: rayEnd should be clamped to the closest cone intersection point when camera is outside volume
	
	const uint sampleCount = 128;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for (uint i = 0; i < sampleCount; ++i)
	{
		float3 L = light.positionWS - P;
		const float dist2 = dot(L, L);
		const float dist = sqrt(dist2);
		L /= dist;

		float SpotFactor = dot(L, light.directionWS);
		float spotCutOff = light.coneAngleCos;

		[branch]
		if (SpotFactor > spotCutOff)
		{
			const float range2 = light.range * light.range;
			const float att = saturate(1.0 - (dist2 / range2));
			float3 attenuation = att * att;
			attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

			[branch]
			if (light.IsCastingShadow())
			{
				float4 ShPos = mul(MatrixArray[light.GetShadowMatrixIndex() + 0], float4(P, 1));
				ShPos.xyz /= ShPos.w;
				float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
				[branch]
				if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
				{
					attenuation *= shadowCascade(light, ShPos.xyz, ShTex.xy, 0);
				}
			}

			attenuation *= GetFogAmount(cameraDistance - marchedDistance);

			accumulation += attenuation;
		}

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}

	accumulation /= sampleCount;

	return max(0, float4(accumulation * light.GetColor().rgb * light.energy, 1));
}