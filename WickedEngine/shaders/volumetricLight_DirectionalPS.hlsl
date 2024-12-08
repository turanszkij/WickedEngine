#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK // fix the lack of depth testing
#define DISABLE_SOFT_SHADOWMAP
#include "volumetricLightHF.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "fogHF.hlsli"
#include "oceanSurfaceHF.hlsli"

half4 main(VertexToPixel input) : SV_Target
{
	ShaderEntity light = load_entity(directional_lights().first_item() + (uint)g_xColor.x);
	
	float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float4 depths = texture_depth.GatherRed(sampler_point_clamp, ScreenCoord);
	float depth = max(depths.x, max(depths.y, max(depths.z, depths.w)));
	float3 P = reconstruct_position(ScreenCoord, depth);
	float3 V = GetCamera().position - P;
	float cameraDistance = length(V);
	V /= cameraDistance;

	// Fix for ocean: because ocean is not in linear depth, we trace it instead
	const ShaderOcean ocean = GetWeather().ocean;
	if(ocean.IsValid() && V.y > 0)
	{
		float3 ocean_surface_pos = intersectPlaneClampInfinite(GetCamera().position, V, float3(0, 1, 0), ocean.water_height);
		float dist = distance(ocean_surface_pos, GetCamera().position);
		if(dist < cameraDistance)
		{
			P = ocean_surface_pos;
			cameraDistance = dist;
		}
	}

	float marchedDistance = 0;
	half3 accumulation = 0;

	const half3 L = light.GetDirection();
	const half scattering = ComputeScattering(saturate(dot(L, -V)));

	float3 rayEnd = GetCamera().position;

	const uint sampleCount = 16;
	const half sampleCount_rcp = rcp((half)sampleCount);
	const float stepSize = length(P - rayEnd) / sampleCount;

	// dither ray start to help with undersampling:
	P = P + V * min(stepSize * dither(input.pos.xy), 10); // limit dithering step to 10 to avoid very large dither on sky

	// Perform ray marching to integrate light volume along view ray:
	[loop]
	for (uint i = 0; i < sampleCount; ++i)
	{
		bool valid = false;

		half3 shadow = 1;
		for (uint cascade = 0; cascade < light.GetShadowCascadeCount(); ++cascade)
		{
			float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(P, 1)).xyz; // ortho matrix, no divide by .w
			float3 shadow_uv = shadow_pos.xyz * float3(0.5f, -0.5f, 0.5f) + 0.5f;

			[branch]
			if (is_saturated(shadow_uv))
			{
				shadow *= shadow_2D(light, shadow_pos, shadow_uv.xy, cascade, input.pos.xy);
				break;
			}
		}

		if (GetFrame().options & OPTION_BIT_VOLUMETRICCLOUDS_CAST_SHADOW)
		{
			shadow *= shadow_2D_volumetricclouds(P);
		}

		// Evaluate sample height for height fog calculation, given 0 for V:
		shadow *= (half)g_xColor.y + GetFogAmount(cameraDistance - marchedDistance, P, 0);
		shadow *= scattering;

		accumulation += shadow;

		marchedDistance += stepSize;
		P = P + V * stepSize;
	}
	accumulation *= sampleCount_rcp;

	half3 atmosphere_transmittance = 1;
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		atmosphere_transmittance = GetAtmosphericLightTransmittance(GetWeather().atmosphere, P, L, texture_transmittancelut);
	}

	return max(0, half4(accumulation * light.GetColor().rgb * atmosphere_transmittance, 1));
}
