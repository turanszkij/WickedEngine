#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Purpose of this shader:
//	- resolves the blend region mask from primitive ID buffer into 8bit unorm (unorm to allow gather later)
//	- detects edges on the blend region mask, outputs thin edge map

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<half2> output_mask : register(u0);
RWTexture2D<uint2> output_edgemap : register(u1);

static const uint THREADCOUNT = 8;
static const int TILE_BORDER = 1;
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared uint cache[TILE_SIZE * TILE_SIZE];
inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

// Straight neighbors first, diagonals after
static const int2 offsets[] = {
	int2(-1, 0),
	int2(1, 0),
	int2(0, -1),
	int2(0, 1),
	
	int2(-1, -1),
	int2(-1, 1),
	int2(1, -1),
	int2(1, 1),
};

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	// preload grid cache:
	//	We only need center pixel for region mask, but we need a padded grid to detect edges, this allows combining both steps into a single pass
	//	in exchange for some extra computation
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER;
	for (uint y = GTid.y; y < TILE_SIZE; y += THREADCOUNT)
	for (uint x = GTid.x; x < TILE_SIZE; x += THREADCOUNT)
	{
		const uint t = coord_to_cache(int2(x, y));
		cache[t] = 0;
		
		const int2 pixel = clamp(tile_upperleft + int2(x, y), 0, postprocess.resolution - 1);
	
		uint primitiveID = texture_primitiveID[pixel];
		if (primitiveID == 0)
			continue;
		
		PrimitiveID prim;
		prim.init();
		prim.unpack(primitiveID);

		ShaderScene scene = GetScene();
	
		if (prim.instanceIndex >= scene.instanceCount)
			continue;
		ShaderMeshInstance inst = load_instance(prim.instanceIndex);
		uint geometryIndex = inst.geometryOffset + prim.subsetIndex;
		if (geometryIndex >= scene.geometryCount)
			continue;
		ShaderGeometry geometry = load_geometry(geometryIndex);
		if (geometry.vb_pos_wind < 0)
			continue;

		if (geometry.materialIndex >= scene.materialCount)
			continue;
		ShaderMaterial material = load_material(geometry.materialIndex);

		const half mesh_blend = material.GetMeshBlend();
		if (mesh_blend > 0)
		{
			cache[t] = pack_half2(half(float((((prim.instanceIndex + 1) * (geometry.materialIndex + 1)) % 255) + 1) / 255.0), saturate(mesh_blend / postprocess.params0.y)); // +1: forced request indicator
		}
	}
	GroupMemoryBarrierWithGroupSync();
	
	const int2 pixel_local = TILE_BORDER + GTid.xy;
	const half2 current = unpack_half2(cache[coord_to_cache(pixel_local)]);
	const half current_id = current.x;

	output_mask[DTid.xy] = current;

	// Find edges by going through the cached grid:
	for (uint i = 0; i < arraysize(offsets); ++i)
	{
		const int2 offset = offsets[i];
		const half id = unpack_half2(cache[coord_to_cache(pixel_local + offset)]).x;
		if (id != current_id)
		{
			// early exit relies on the straight neighbors having priority in iteration, and diagonal neighbors after because they are farther
			output_edgemap[DTid.xy] = clamp((int2)DTid.xy + offset, 0, postprocess.resolution - 1);
			return;
		}
	}
}
