#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

// This shader reads primitiveID and writes depth and linear depth and a simple mipchain for them
//	It also does material binning when requested
//	It can run in uniform and divergent manner based on previous primitiveID classification

StructuredBuffer<PrimitiveVisibilityTile> primitive_binned_tiles : register(t0);

groupshared uint local_bin_mask;
groupshared uint local_bin_execution_mask_0[SHADERTYPE_BIN_COUNT + 1];
groupshared uint local_bin_execution_mask_1[SHADERTYPE_BIN_COUNT + 1];

groupshared float shared_depth[VISIBILITY_BLOCKSIZE / 2][VISIBILITY_BLOCKSIZE / 2];

RWStructuredBuffer<IndirectDispatchArgs> output_bins : register(u0);
RWStructuredBuffer<VisibilityTile> output_binned_tiles : register(u1);

RWTexture2D<float> output_depth_mip0 : register(u3);
RWTexture2D<float> output_depth_mip1 : register(u4);
RWTexture2D<float> output_depth_mip2 : register(u5);
RWTexture2D<float> output_depth_mip3 : register(u6);
RWTexture2D<float> output_depth_mip4 : register(u7);

[numthreads(VISIBILITY_BLOCKSIZE * VISIBILITY_BLOCKSIZE, 1, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
#ifdef MATERIAL_BINNING
	if (groupIndex <= SHADERTYPE_BIN_COUNT)
	{
		if (groupIndex == 0)
		{
			local_bin_mask = 0;
		}
#ifndef PRIMITIVEID_UNIFORM
		// Note: no need to track execution mask for uniform tiles
		local_bin_execution_mask_0[groupIndex] = 0;
		local_bin_execution_mask_1[groupIndex] = 0;
#endif // PRIMITIVEID_UNIFORM
	}
	GroupMemoryBarrierWithGroupSync();
#endif // MATERIAL_BINNING

	const uint2 GTid = remap_lane_8x8(groupIndex);

#ifdef PRIMITIVEID_UNIFORM
	const uint tile_offset = 0;
#else
	const uint tile_offset = GetCamera().visibility_tilecount_flat;
#endif // PRIMITIVEID_UNIFORM

	PrimitiveVisibilityTile primitive_tile = primitive_binned_tiles[tile_offset + Gid];

	const uint2 tileID = unpack_pixel(primitive_tile.visibility_tile_id);
	const uint2 pixel = tileID * VISIBILITY_BLOCKSIZE + GTid;

#ifdef PRIMITIVEID_UNIFORM
	const uint primitiveID = primitive_tile.primitiveID;
#else
	const bool pixel_valid = (pixel.x < GetCamera().internal_resolution.x) && (pixel.y < GetCamera().internal_resolution.y);
	const uint primitiveID = pixel_valid ? texture_primitiveID[pixel] : 0;
#endif // PRIMITIVEID_UNIFORM
	
	RayDesc ray = CreateCameraRay(pixel);

	float depth = 0; // sky default
	uint bin = SHADERTYPE_BIN_COUNT; // sky default

	[branch]
	if (primitiveID != 0)
	{
		PrimitiveID prim;
		prim.init();
		prim.unpack(primitiveID);

		Surface surface;
		surface.init();

		[branch]
		if (surface.load(prim, ray.Origin, ray.Direction))
		{
			float4 tmp = mul(GetCamera().view_projection, float4(surface.P, 1));
			tmp.xyz /= max(0.0001, tmp.w); // max: avoid nan
			depth = saturate(tmp.z); // saturate: avoid blown up values

#ifdef MATERIAL_BINNING
			bin = surface.material.GetShaderType();
#endif // MATERIAL_BINNING
		}
	}

#ifdef MATERIAL_BINNING

#ifndef PRIMITIVEID_UNIFORM
	// Note: no need to track execution mask for uniform tiles
	if (groupIndex < 32)
	{
		InterlockedOr(local_bin_execution_mask_0[bin], 1u << groupIndex);
	}
	else
	{
		InterlockedOr(local_bin_execution_mask_1[bin], 1u << (groupIndex - 32u));
	}
#endif // PRIMITIVEID_UNIFORM

	uint wave_local_bin_mask = WaveActiveBitOr(1u << bin);
	if (WaveIsFirstLane())
	{
		InterlockedOr(local_bin_mask, wave_local_bin_mask);
	}

	GroupMemoryBarrierWithGroupSync();
	if (groupIndex < SHADERTYPE_BIN_COUNT + 1)
	{
		if (local_bin_mask & (1u << groupIndex))
		{
			uint bin_tile_list_offset = groupIndex * GetCamera().visibility_tilecount_flat;
			uint bucket_index = groupIndex;

#ifndef PRIMITIVEID_UNIFORM
			bin_tile_list_offset += GetCamera().visibility_tilecount_flat * (SHADERTYPE_BIN_COUNT + 1);
			bucket_index += SHADERTYPE_BIN_COUNT + 1;
#endif // PRIMITIVEID_UNIFORM

			uint tile_offset = 0;
			InterlockedAdd(output_bins[bucket_index].ThreadGroupCountX, 1, tile_offset);

			VisibilityTile tile;
			tile.visibility_tile_id = primitive_tile.visibility_tile_id;
#ifdef PRIMITIVEID_UNIFORM
			tile.execution_mask_or_primitiveID = primitive_tile.primitiveID;
#else
			tile.execution_mask_or_primitiveID = uint64_t(local_bin_execution_mask_0[groupIndex]) | (uint64_t(local_bin_execution_mask_1[groupIndex]) << uint64_t(32));
#endif // PRIMITIVEID_UNIFORM
			output_binned_tiles[bin_tile_list_offset + tile_offset] = tile;
		}
	}
#endif // MATERIAL_BINNING

#ifdef PRIMITIVEID_UNIFORM
	if (primitiveID == 0)
		return; // depths are pre-cleared for sky in whole, no need to continue here if the whole tile is uniform
#endif // PRIMITIVEID_UNIFORM

	// mip0
	output_depth_mip0[pixel] = depth;

	// mip1
	float x_depth = QuadReadAcrossX(depth);
	float min_d_quad = min(depth, x_depth);
	float y_depth = QuadReadAcrossY(min_d_quad);
	float min_d_mip1 = min(min_d_quad, y_depth);

	// LDS for mip2 an onward:
	if ((GTid.x & 1) == 0 && (GTid.y & 1) == 0)
	{
		output_depth_mip1[pixel / 2] = min_d_mip1;
		uint2 lds_coord = GTid / 2; 
		shared_depth[lds_coord.y][lds_coord.x] = min_d_mip1;
	}
	GroupMemoryBarrierWithGroupSync();

	// mip2
	if ((GTid.x & 3) == 0 && (GTid.y & 3) == 0)
	{
		uint2 lds_coord = GTid / 2;

		float d0 = shared_depth[lds_coord.y + 0][lds_coord.x + 0];
		float d1 = shared_depth[lds_coord.y + 0][lds_coord.x + 1];
		float d2 = shared_depth[lds_coord.y + 1][lds_coord.x + 0];
		float d3 = shared_depth[lds_coord.y + 1][lds_coord.x + 1];

		float min_d = min(min(d0, d1), min(d2, d3));

		shared_depth[lds_coord.y][lds_coord.x] = min_d;

		output_depth_mip2[pixel / 4] = min_d;
	}
	GroupMemoryBarrierWithGroupSync();

	// mip3
	if (GTid.x == 0 && GTid.y == 0)
	{
		float d0 = shared_depth[0][0];
		float d1 = shared_depth[0][2];
		float d2 = shared_depth[2][0];
		float d3 = shared_depth[2][2];

		float min_d = min(min(d0, d1), min(d2, d3));

		shared_depth[0][0] = min_d;

		output_depth_mip3[pixel / 8] = min_d;
	}

	// mip4
	if (GTid.x == 0 && GTid.y == 0)
	{
		output_depth_mip4[pixel / 16] = shared_depth[0][0];
	}

}
