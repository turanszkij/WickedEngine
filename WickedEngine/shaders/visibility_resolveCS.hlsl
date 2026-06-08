#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

// This shader reads primitiveID and writes depth and linear depth and a simple mipchain for them
//	It also does material binning when requested
//	It can run in uniform and divergent manner based on previous primitiveID classification

Texture2D<uint> input_primitiveID : register(t0);
StructuredBuffer<PrimitiveVisibilityTile> primitive_binned_tiles : register(t1);

groupshared uint local_bin_mask;
groupshared uint local_bin_execution_mask_0[SHADERTYPE_BIN_COUNT + 1];
groupshared uint local_bin_execution_mask_1[SHADERTYPE_BIN_COUNT + 1];

RWStructuredBuffer<IndirectDispatchArgs> output_bins : register(u0);
RWStructuredBuffer<VisibilityTile> output_binned_tiles : register(u1);

RWTexture2D<float> output_depth_mip0 : register(u3);
RWTexture2D<float> output_depth_mip1 : register(u4);
RWTexture2D<float> output_depth_mip2 : register(u5);
RWTexture2D<float> output_depth_mip3 : register(u6);
RWTexture2D<float> output_depth_mip4 : register(u7);

RWTexture2D<float> output_lineardepth_mip0 : register(u8);
RWTexture2D<float> output_lineardepth_mip1 : register(u9);
RWTexture2D<float> output_lineardepth_mip2 : register(u10);
RWTexture2D<float> output_lineardepth_mip3 : register(u11);
RWTexture2D<float> output_lineardepth_mip4 : register(u12);

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
#ifdef MATERIAL_BINNING
	if (groupIndex <= SHADERTYPE_BIN_COUNT)
	{
		if (groupIndex == 0)
		{
			local_bin_mask = 0;
		}
		local_bin_execution_mask_0[groupIndex] = 0;
		local_bin_execution_mask_1[groupIndex] = 0;
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
	const uint primitiveID = input_primitiveID[pixel];
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
	if (groupIndex < 32)
	{
		InterlockedOr(local_bin_execution_mask_0[bin], 1u << groupIndex);
	}
	else
	{
		InterlockedOr(local_bin_execution_mask_1[bin], 1u << (groupIndex - 32u));
	}

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
			tile.primitiveID = primitive_tile.primitiveID;
			tile.execution_mask = uint64_t(local_bin_execution_mask_0[groupIndex]) | (uint64_t(local_bin_execution_mask_1[groupIndex]) << uint64_t(32));
			output_binned_tiles[bin_tile_list_offset + tile_offset] = tile;
		}
	}
#endif // MATERIAL_BINNING

	if (primitiveID == 0)
		return; // depths are pre-cleared for sky in whole, no need to continue here

	// Downsample depths:
	output_depth_mip0[pixel] = depth;
	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		output_depth_mip1[pixel / 2] = depth;
	}
	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		output_depth_mip2[pixel / 4] = depth;
	}
	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		output_depth_mip3[pixel / 8] = depth;
	}
	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		output_depth_mip4[pixel / 16] = depth;
	}

	float lineardepth = compute_lineardepth(depth) * GetCamera().z_far_rcp;
	output_lineardepth_mip0[pixel] = lineardepth;
	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		output_lineardepth_mip1[pixel / 2] = lineardepth;
	}
	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		output_lineardepth_mip2[pixel / 4] = lineardepth;
	}
	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		output_lineardepth_mip3[pixel / 8] = lineardepth;
	}
	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		output_lineardepth_mip4[pixel / 16] = lineardepth;
	}

}
