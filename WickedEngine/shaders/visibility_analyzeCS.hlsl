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

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint2 Gid : SV_GroupID, uint2 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint2 pixel = DTid;
	
#ifdef PRIMITIVEID_MSAA
	uint primitiveID = input_primitiveID.Load(pixel, 0);
	output_primitiveID[pixel] = primitiveID;
#else
	uint primitiveID = input_primitiveID[pixel];
#endif // PRIMITIVEID_MSAA

	const uint primitiveID_firstlane = WaveReadLaneFirst(primitiveID);
	const bool is_uniform = WaveActiveAllTrue(primitiveID == primitiveID_firstlane);

	if (groupIndex == 0)
	{
		if (is_uniform)
		{
			uint prevCount = 0;
			InterlockedAdd(output_primitive_bins[0].ThreadGroupCountX, 1, prevCount);

			PrimitiveVisibilityTile tile;
			tile.visibility_tile_id = pack_pixel(Gid.xy);
			tile.primitiveID = primitiveID_firstlane;
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
