#include "globals.hlsli"
#include "skyAtmosphere.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float3> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
	
	if (depth > 0.0)
	{
		output[DTid.xy] = float3(0.0, 0.0, 0.0);
		return;
	}
	
	float2 screenPosition = uv_to_clipspace(uv);
	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 depthWorldPosition = reconstruct_position(uv, depth);

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);
	
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
	
	float3 worldPosition = GetCameraPlanetPos(atmosphere, rayOrigin);
	float3 worldDirection = rayDirection;

	float3 sunDirection = GetSunDirection();
	float3 sunIlluminance = GetSunColor();
	
	float3 luminance = GetSunLuminance(worldPosition, worldDirection, sunDirection, sunIlluminance, atmosphere, texture_transmittancelut);
	
    // Output
	output[DTid.xy] = luminance;
}
