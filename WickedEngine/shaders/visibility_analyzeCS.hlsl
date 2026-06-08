#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

// This shader analyzes the primitiveID to determine for each tile whether the primitiveID is uniform or divergent
//	It classifies tiles based on this to allow simpler shader execution for tiles which contain uniform primitiveID values
//	As a little extra, it will write out a single sampled primitiveID if the source was multisampled to avoid doing it in a separate pass

#ifdef PRIMITIVEID_MSAA
Texture2DMS<uint> input_primitiveID : register(t0);
#else
Texture2D<uint> input_primitiveID : register(t0);
#endif // PRIMITIVEID_MSAA

RWStructuredBuffer<IndirectDispatchArgs> output_primitive_bins : register(u0);
RWStructuredBuffer<PrimitiveVisibilityTile> output_primitive_binned_tiles : register(u1);

#ifdef PRIMITIVEID_MSAA
RWTexture2D<uint> output_primitiveID : register(u2);
#endif // PRIMITIVEID_MSAA

groupshared uint is_group_uniform;
groupshared uint group_primitiveID;

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint2 Gid : SV_GroupID, uint2 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint2 pixel = DTid;
	const bool pixel_valid = (pixel.x < GetCamera().internal_resolution.x) && (pixel.y < GetCamera().internal_resolution.y);
	
#ifdef PRIMITIVEID_MSAA
	uint primitiveID = pixel_valid ? input_primitiveID.Load(pixel, 0) : 0;
	if (pixel_valid) output_primitiveID[pixel] = primitiveID;
#else
	uint primitiveID = pixel_valid ? input_primitiveID[pixel] : 0;
#endif // PRIMITIVEID_MSAA

	if (groupIndex == 0)
	{
		is_group_uniform = 1;
		group_primitiveID = primitiveID;
	}
	GroupMemoryBarrierWithGroupSync();

	const bool group_primitiveID_match = !pixel_valid || (primitiveID == group_primitiveID);
	const bool is_wave_uniform = WaveActiveAllTrue(group_primitiveID_match);

	if (WaveIsFirstLane())
	{
		InterlockedAnd(is_group_uniform, is_wave_uniform ? 1 : 0);
	}
	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		if (is_group_uniform)
		{
			uint prevCount = 0;
			InterlockedAdd(output_primitive_bins[0].ThreadGroupCountX, 1, prevCount);

			PrimitiveVisibilityTile tile;
			tile.visibility_tile_id = pack_pixel(Gid.xy);
			tile.primitiveID = group_primitiveID;
			output_primitive_binned_tiles[prevCount] = tile;
		}
		else
		{
			uint prevCount = 0;
			InterlockedAdd(output_primitive_bins[1].ThreadGroupCountX, 1, prevCount);

			PrimitiveVisibilityTile tile;
			tile.visibility_tile_id = pack_pixel(Gid.xy);
			tile.primitiveID = 0;
			output_primitive_binned_tiles[GetCamera().visibility_tilecount_flat + prevCount] = tile;
		}
	}

}
