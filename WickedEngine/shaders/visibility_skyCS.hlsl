#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "skyHF.hlsli"

ConstantBuffer<ShaderTypeBin> bin : register(b10);
StructuredBuffer<uint> binned_pixels : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= bin.count)
		return;

	uint2 pixel = unpack_pixel(binned_pixels[bin.offset + DTid.x]);

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	float3 envColor = 0;

	[branch]
	if (IsStaticSky())
	{
		// We have envmap information in a texture:
		envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.Direction, 0).rgb);
	}
	else
	{
		envColor = GetDynamicSkyColor(ray.Direction);
	}

	output[pixel] = float4(envColor, 1);
}
