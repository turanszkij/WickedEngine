#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "skyHF.hlsli"

ConstantBuffer<ShaderTypeBin> bin : register(b10);
StructuredBuffer<uint> binned_tiles : register(t0);
Texture2D<uint> texture_shadertypes : register(t1);

RWTexture2D<float4> output : register(u0);

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	uint2 GTid = remap_lane_8x8(groupIndex);
	uint2 pixel = unpack_pixel(binned_tiles[bin.offset + Gid.x]) * VISIBILITY_BLOCKSIZE + GTid;

	// Because we bin whole tiles, we check if the current pixel matches the tile's shaderType
	if (texture_shadertypes[pixel] != bin.shaderType)
	{
		return;
	}

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
