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
	uint clipmap_index = clamp((uint)g_xColor.x, 0, VOXEL_GI_CLIPMAP_COUNT - 1);
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];

	uint3 coord = unflatten3D(input[0], GetFrame().vxgi.resolution);
	uint3 pixel = coord;
	pixel.y += clipmap_index * GetFrame().vxgi.resolution;

	float4 face_colors[6] = {
		texture_voxelgi[pixel + uint3(0 * GetFrame().vxgi.resolution, 0, 0)],
		texture_voxelgi[pixel + uint3(1 * GetFrame().vxgi.resolution, 0, 0)],
		texture_voxelgi[pixel + uint3(2 * GetFrame().vxgi.resolution, 0, 0)],
		texture_voxelgi[pixel + uint3(3 * GetFrame().vxgi.resolution, 0, 0)],
		texture_voxelgi[pixel + uint3(4 * GetFrame().vxgi.resolution, 0, 0)],
		texture_voxelgi[pixel + uint3(5 * GetFrame().vxgi.resolution, 0, 0)],
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
			tri[j].pos = float4(coord, 1);
			tri[j].pos.xyz = tri[j].pos.xyz * GetFrame().vxgi.resolution_rcp * 2 - 1;
			tri[j].pos.y = -tri[j].pos.y;
			tri[j].pos.xyz *= GetFrame().vxgi.resolution;
			tri[j].pos.xyz += (-CUBE[i + j].xyz * 0.5 + 0.5 - float3(0, 1, 0)) * 2;
		}

		float3 facenormal = -normalize(cross(tri[2].pos.xyz - tri[1].pos.xyz, tri[1].pos.xyz - tri[0].pos.xyz));
		float4 color = face_colors[(uint)cubemap_to_uv(facenormal).z];
		if (color.a == 0)
		{
			// just to avoid invisible cube faces:
			color = float4(0, 0, 0, 1);
		}

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
