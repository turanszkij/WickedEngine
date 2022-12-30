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
	uint3 coord = unflatten3D(input[0], GetFrame().vxgi.resolution);

	uint clipmap_index = 0;
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];

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
			tri[j].pos.xyz *= clipmap.voxelSize;
		}

		float3 facenormal = -normalize(cross(tri[2].pos.xyz - tri[1].pos.xyz, tri[1].pos.xyz - tri[0].pos.xyz));

		uint3 pixel = coord;
		pixel.x += cubemap_to_uv(facenormal).z * GetFrame().vxgi.resolution;
		pixel.y += clipmap_index * GetFrame().vxgi.resolution;
		float4 color = texture_voxelgi[pixel];

		if (color.a > 0)
		{
			for (j = 0; j < 3; ++j)
			{
				tri[j].pos = mul(g_xTransform, float4(tri[j].pos.xyz, 1));
				tri[j].col = color;
				tri[j].col *= g_xColor;
				output.Append(tri[j]);
			}
		}

		output.RestartStrip();
	}
}
