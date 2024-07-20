#ifndef WI_SHADERINTEROP_TERRAIN_H
#define WI_SHADERINTEROP_TERRAIN_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct ShaderTerrainChunk
{
	int heightmap;
	uint materialID;

	void init()
	{
		heightmap = -1;
		materialID = 0;
	}
};
struct alignas(16) ShaderTerrain
{
	float3 center_chunk_pos;
	float chunk_size;

	int chunk_buffer;
	int chunk_buffer_range;
	float min_height;
	float max_height;

	void init()
	{
		center_chunk_pos = float3(0, 0, 0);
		chunk_size = 0;
		chunk_buffer = -1;
		chunk_buffer_range = 0;
	}
};

#endif // WI_SHADERINTEROP_TERRAIN_H
