#include "globals.hlsli"
#include "cube.hlsli"
#include "voxelHF.hlsli"

// #ifndef VOXELIZATION_GEOMETRY_SHADER_ENABLED

// struct VSOut
// {
// 	float4 pos : SV_POSITION;
// 	float4 col : TEXCOORD;
// };

// VSOut main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
// {
// 	VSOut Out;
// 	Out.pos = 0;
// 	Out.col = 0;

// 	const uint voxels_per_clip = GetFrame().vxgi.resolution * GetFrame().vxgi.resolution * GetFrame().vxgi.resolution;

// 	uint clipmap_index = (uint)g_xColor.x;
// 	uint voxel_index = instanceID;
// 	bool min_clip = false;

// 	if (clipmap_index == VXGI_CLIPMAP_COUNT)
// 	{
// 		min_clip = true;
// 		voxel_index = instanceID % voxels_per_clip;
// 		clipmap_index = instanceID / voxels_per_clip;
// 	}

// 	const uint3 coord = unflatten3D(voxel_index, GetFrame().vxgi.resolution);

// 	const uint3 pixel_base = uint3(coord.x, coord.y + clipmap_index * GetFrame().vxgi.resolution, coord.z);

// 	float4 face_colors[6];
// 	Texture3D<float4> voxels = bindless_textures3D[descriptor_index(GetFrame().vxgi.texture_radiance)];
// 	[unroll]
// 	for (uint face = 0; face < 6; ++face)
// 	{
// 		face_colors[face] = voxels[pixel_base + uint3(face * GetFrame().vxgi.resolution, 0, 0)];
// 	}

// 	bool has_color = false;
// 	[unroll]
// 	for (uint face = 0; face < 6; ++face)
// 	{
// 		has_color |= face_colors[face].a > 0;
// 	}

// 	if (min_clip && clipmap_index > 0)
// 	{
// 		VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];
// 		float3 P = GetFrame().vxgi.clipmap_to_world((coord - 0.5) / (GetFrame().vxgi.resolution - 2), clipmap);
// 		VoxelClipMap clipmap_below = GetFrame().vxgi.clipmaps[clipmap_index - 1];
// 		float3 diff = (P - clipmap_below.center) * GetFrame().vxgi.resolution_rcp / clipmap_below.voxelSize;
// 		float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
// 		if (is_saturated(uvw))
// 		{
// 			has_color = false;
// 		}
// 	}

// 	if (!has_color)
// 	{
// 		Out.col = float4(0, 0, 0, -1);
// 		return Out;
// 	}

// 	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];

// 	float3 uvw_center = (coord + 0.5) * GetFrame().vxgi.resolution_rcp;
// 	float3 center = uvw_center * 2 - 1;
// 	center.y *= -1;
// 	center *= GetFrame().vxgi.resolution;

// 	const uint tri_base = (vertexID / 3) * 3;
// 	const uint tri_vertex = vertexID % 3;

// 	float3 tri_positions[3];
// 	[unroll]
// 	for (uint i = 0; i < 3; ++i)
// 	{
// 		tri_positions[i] = center + (-CUBE[tri_base + i].xyz);
// 	}

// 	float3 facenormal = -normalize(cross(tri_positions[2] - tri_positions[1], tri_positions[1] - tri_positions[0]));
// 	uint face_index = (uint)cubemap_to_uv(facenormal).z;
// 	float4 color = face_colors[face_index];
// 	if (color.a == 0)
// 	{
// 		color = float4(0, 0, 0, 1);
// 	}
// 	color.a = max(color.a, 0.001f);

// 	float3 position = tri_positions[tri_vertex];
// 	position *= clipmap.voxelSize;
// 	position += clipmap.center;

// 	Out.pos = mul(g_xTransform, float4(position, 1));
// 	Out.col = color;
// 	return Out;
// }

// #else

uint main(uint vertexID : SV_VERTEXID) : VERTEXID
{
	return vertexID;
}

// #endif // VOXELIZATION_GEOMETRY_SHADER_ENABLED
