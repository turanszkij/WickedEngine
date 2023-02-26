#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK // fix the lack of depth testing
#include "volumetricLightHF.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "fogHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderEntity light = load_entity(GetFrame().lightarray_offset + (uint)g_xColor.x);

	if (!light.IsCastingShadow())
	{
		// Dirlight volume has no meaning without shadows!!
		return 0;
	}

	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float depth = max(input.pos.z, texture_depth.SampleLevel(sampler_point_clamp, ScreenCoord, 0));
	float3 P = reconstruct_position(ScreenCoord, depth);
	float3 V = GetCamera().position - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	float marchedDistance = 0;
	float3 accumulation = 0;

	const float3 L = light.GetDirection();
	const float scattering = ComputeScattering(saturate(dot(L, -V)));

	float3 rayEnd = GetCamera().position;

	const uint sampleCount = 16;
	const float stepSize = length(P - rayEnd) / sampleCount;

	// dither ray start to help with undersampling:
	P = P + V * min(stepSize * dither(input.pos.xy), 10); // limit dithering step to 10 to avoid very large dither on sky

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for (uint i = 0; i < sampleCount; ++i)
	{
		bool valid = false;

		float3 shadow = 1;
		for (uint cascade = 0; cascade < light.GetShadowCascadeCount(); ++cascade)
		{
			float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(P, 1)).xyz; // ortho matrix, no divide by .w
			float3 shadow_uv = shadow_pos.xyz * float3(0.5f, -0.5f, 0.5f) + 0.5f;

			[branch]
			if (is_saturated(shadow_uv))
			{
				shadow *= shadow_2D(light, shadow_pos, shadow_uv.xy, cascade);
				break;
			}
		}

		if (GetFrame().options & OPTION_BIT_VOLUMETRICCLOUDS_CAST_SHADOW)
		{
			shadow *= shadow_2D_volumetricclouds(P);
		}

		// Evaluate sample height for height fog calculation, given 0 for V:
		shadow *= GetFogAmount(cameraDistance - marchedDistance, P, 0);
		shadow *= scattering;

		accumulation += shadow;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}
	accumulation /= sampleCount;

	float3 atmosphere_transmittance = 1;
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		atmosphere_transmittance = GetAtmosphericLightTransmittance(GetWeather().atmosphere, P, L, texture_transmittancelut);
	}

	return max(0, float4(accumulation * light.GetColor().rgb * atmosphere_transmittance, 1));
}
