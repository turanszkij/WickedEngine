#include "globals.hlsli"
#include "cube.hlsli"

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD;
};

[maxvertexcount(36)]
void main(
	point uint input[1] : VERTEXID,
	inout TriangleStream<GSOutput> output
)
{
	uint voxel_index = input[0];
	uint clipmap_index = (uint)g_xColor.x;

	bool min_clip = false;
	if (clipmap_index == VXGI_CLIPMAP_COUNT)
	{
		min_clip = true;
		const uint voxel_count = GetFrame().vxgi.resolution * GetFrame().vxgi.resolution * GetFrame().vxgi.resolution;
		voxel_index = input[0] % voxel_count;
		clipmap_index = input[0] / voxel_count;
	}

	uint3 coord = unflatten3D(voxel_index, GetFrame().vxgi.resolution);
	float3 uvw = (coord + 0.5) * GetFrame().vxgi.resolution_rcp;
	float3 center = uvw * 2 - 1;
	center.y *= -1;
	center *= GetFrame().vxgi.resolution;

	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];

	if (min_clip && clipmap_index > 0)
	{
		float3 P = GetFrame().vxgi.clipmap_to_world((coord - 0.5) / (GetFrame().vxgi.resolution - 2), clipmap);
		VoxelClipMap clipmap_below = GetFrame().vxgi.clipmaps[clipmap_index - 1];
		float3 diff = (P - clipmap_below.center) * GetFrame().vxgi.resolution_rcp / clipmap_below.voxelSize;
		float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
		if (is_saturated(uvw))
			return;
	}

	uint3 pixel = coord;
	pixel.y += clipmap_index * GetFrame().vxgi.resolution;

	Texture3D<float4> voxels = bindless_textures3D[GetFrame().vxgi.texture_radiance];
	float4 face_colors[6] = {
		voxels[pixel + uint3(0 * GetFrame().vxgi.resolution, 0, 0)],
		voxels[pixel + uint3(1 * GetFrame().vxgi.resolution, 0, 0)],
		voxels[pixel + uint3(2 * GetFrame().vxgi.resolution, 0, 0)],
		voxels[pixel + uint3(3 * GetFrame().vxgi.resolution, 0, 0)],
		voxels[pixel + uint3(4 * GetFrame().vxgi.resolution, 0, 0)],
		voxels[pixel + uint3(5 * GetFrame().vxgi.resolution, 0, 0)],
	};

	if (
		face_colors[0].a == 0 &&
		face_colors[1].a == 0 &&
		face_colors[2].a == 0 &&
		face_colors[3].a == 0 &&
		face_colors[4].a == 0 &&
		face_colors[5].a == 0
		)
	{
		return;
	}

	for (uint i = 0; i < 36; i += 3)
	{
		GSOutput tri[3];
		uint j = 0;
		for (j = 0; j < 3; ++j)
		{
			tri[j].pos = float4(center, 1);
			tri[j].pos.xyz += -CUBE[i + j].xyz;
		}

		float3 facenormal = -normalize(cross(tri[2].pos.xyz - tri[1].pos.xyz, tri[1].pos.xyz - tri[0].pos.xyz));
		float4 color = face_colors[(uint)cubemap_to_uv(facenormal).z];
		if (color.a == 0)
		{
			// just to avoid invisible cube faces:
			color = float4(0, 0, 0, 1);
		}

		if(color.a > 0)
		for (j = 0; j < 3; ++j)
		{
			tri[j].pos.xyz *= clipmap.voxelSize;
			tri[j].pos.xyz += clipmap.center;
			tri[j].pos = mul(g_xTransform, float4(tri[j].pos.xyz, 1));
			tri[j].col = color;
			output.Append(tri[j]);
		}

		output.RestartStrip();
	}
}
