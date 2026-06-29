#define SURFACE_LOAD_QUAD_DERIVATIVES
#define SURFACE_LOAD_ENABLE_WIND
#define SVT_FEEDBACK
#define TEXTURE_SLOT_NONUNIFORM
#define PRIMITIVEID_FROM_MESHLET_OPTIMIZED
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "surfaceHF.hlsli"
#include "shadingHF.hlsli"

// This shader extracts per-pixel surface attributes normal and roughness based on primitiveID

struct VisibilityPushConstants
{
	uint global_tile_offset;
};
PUSHCONSTANT(push, VisibilityPushConstants);

StructuredBuffer<VisibilityTile> binned_tiles : register(t0);

RWTexture2D<half3> output_normals_roughness : register(u0);

[numthreads(VISIBILITY_BLOCKSIZE * VISIBILITY_BLOCKSIZE, 1, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint tile_offset = push.global_tile_offset + Gid.x;
	VisibilityTile tile = binned_tiles[tile_offset];
	const uint2 GTid = remap_lane_quads(groupIndex);
	const uint2 tileID = unpack_pixel(tile.visibility_tile_id);
	const uint2 pixel = tileID * VISIBILITY_BLOCKSIZE + GTid;

	ShaderCamera camera = GetCamera();
	const uint entity_flat_tile_index = flatten2D(tileID / VISIBILITY_TILED_CULLING_GRANULARITY, camera.entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	const float2 uv = ((float2)pixel + 0.5) * camera.internal_resolution_rcp;
	RayDesc ray = CreateCameraRay(pixel);
	float3 rayDirection_quad_x = QuadReadAcrossX(ray.Direction);
	float3 rayDirection_quad_y = QuadReadAcrossY(ray.Direction);

#ifdef PRIMITIVEID_UNIFORM
	const uint primitiveID = tile.shaderType_or_primitiveID;
#else
	const uint primitiveID = texture_primitiveID[pixel];
#endif // PRIMITIVEID_UNIFORM

	PrimitiveID prim;
	prim.init();
	prim.unpack(primitiveID);

#ifndef PRIMITIVEID_UNIFORM
	[branch] if (prim.shaderType != tile.shaderType_or_primitiveID) return;
#endif // PRIMITIVEID_UNIFORM

	Surface surface;
	surface.init();
	surface.pixel = pixel.xy;
	surface.screenUV = uv;

	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction, rayDirection_quad_x, rayDirection_quad_y, entity_flat_tile_index))
	{
		return;
	}

	// Write out sampleable attributes for post processing into textures:
#ifdef CLEARCOAT
	// Clearcoat must write out the top layer's normal and roughness to match the specs:
	output_normals_roughness[pixel] = half3(encode_normal(surface.clearcoat.N), surface.clearcoat.roughness);
#else
	output_normals_roughness[pixel] = half3(encode_normal(surface.N), surface.roughness);
#endif // CLEARCOAT

}
