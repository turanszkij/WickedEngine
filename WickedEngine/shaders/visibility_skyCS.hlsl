#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "skyHF.hlsli"
#include "fogHF.hlsli"

struct VisibilityPushConstants
{
	uint global_tile_offset;
};
PUSHCONSTANT(push, VisibilityPushConstants);

StructuredBuffer<VisibilityTile> binned_tiles : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint tile_offset = push.global_tile_offset + Gid.x;
	VisibilityTile tile = binned_tiles[tile_offset];
	[branch]
	if (!tile.check_thread_valid(groupIndex))
		return;
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = unpack_pixel(tile.visibility_tile_id) * VISIBILITY_BLOCKSIZE + GTid;
	const float2 uv = ((float2) pixel + 0.5) * GetCamera().internal_resolution_rcp;

	[branch]
	if (!GetCamera().is_uv_inside_scissor(uv))
	{
		output[pixel] = 0;
		return;
	}

	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	float3 envColor = 0;

	[branch]
	if (IsStaticSky())
	{
		// We have envmap information in a texture:
		envColor = GetStaticSkyColor(ray.Direction);
	}
	else
	{
		bool highQuality = GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY;
		bool perPixelNoise = GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED;

		// Shadows disabled due to parallel computation along shadowmap rendering
		envColor = GetDynamicSkyColor(pixel, ray.Direction, true, false, false, highQuality, perPixelNoise, false);
	}

	output[pixel] = float4(envColor, 1);
}
