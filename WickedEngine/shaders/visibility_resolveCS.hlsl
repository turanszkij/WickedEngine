#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

#ifdef VISIBILITY_MSAA
Texture2DMS<uint> input_primitiveID : register(t0);
#else
Texture2D<uint> input_primitiveID : register(t0);
#endif // VISIBILITY_MSAA

groupshared uint local_bin_mask;
groupshared uint local_bin_execution_mask_0[SHADERTYPE_BIN_COUNT + 1];
groupshared uint local_bin_execution_mask_1[SHADERTYPE_BIN_COUNT + 1];

RWStructuredBuffer<ShaderTypeBin> output_bins : register(u0);
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

#ifdef VISIBILITY_MSAA
RWTexture2D<uint> output_primitiveID : register(u13);
#endif // VISIBILITY_MSAA

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
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

	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = Gid.xy * VISIBILITY_BLOCKSIZE + GTid.xy;
	const bool pixel_valid = (pixel.x < GetCamera().internal_resolution.x) && (pixel.y < GetCamera().internal_resolution.y);

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);
	
#ifdef VISIBILITY_MSAA
	uint primitiveID = input_primitiveID.Load(pixel, 0);
#else
	uint primitiveID = input_primitiveID[pixel];
#endif // VISIBILITY_MSAA

#ifdef VISIBILITY_MSAA
	output_primitiveID[pixel] = primitiveID;
#endif // VISIBILITY_MSAA

	float depth = 1; // invalid
	uint bin = ~0u; // invalid
	if (pixel_valid)
	{
		[branch]
		if (any(primitiveID))
		{
			PrimitiveID prim;
			prim.unpack(primitiveID);

			Surface surface;
			surface.init();

			[branch]
			if (surface.load(prim, ray.Origin, ray.Direction))
			{
				float4 tmp = mul(GetCamera().view_projection, float4(surface.P, 1));
				tmp.xyz /= max(0.0001, tmp.w); // max: avoid nan
				depth = saturate(tmp.z); // saturate: avoid blown up values

				bin = surface.material.shaderType;
			}
		}
		else
		{
			// sky:
			depth = 0;
			bin = SHADERTYPE_BIN_COUNT;
		}
		if (groupIndex < 32)
		{
			InterlockedOr(local_bin_execution_mask_0[bin], 1u << groupIndex);
		}
		else
		{
			InterlockedOr(local_bin_execution_mask_1[bin], 1u << (groupIndex - 32u));
		}
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
			uint tile_offset = 0;
			InterlockedAdd(output_bins[groupIndex].dispatchX, 1, tile_offset);

			VisibilityTile tile;
			tile.visibility_tile_id = pack_pixel(Gid.xy);
			tile.entity_flat_tile_index = flatten2D(Gid.xy / VISIBILITY_TILED_CULLING_GRANULARITY, GetCamera().entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;
			tile.execution_mask_0 = local_bin_execution_mask_0[groupIndex];
			tile.execution_mask_1 = local_bin_execution_mask_1[groupIndex];
			output_binned_tiles[bin_tile_list_offset + tile_offset] = tile;
		}
	}

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
