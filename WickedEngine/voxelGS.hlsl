#include "globals.hlsli"

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD;
};

[maxvertexcount(14)]
void main(
	point GSOutput input[1],
	inout TriangleStream< GSOutput > output
)
{
	[branch]
	if (input[0].col.a > 0)
	{
		for (uint i = 0; i < 14; i++)
		{
			GSOutput element;
			element.pos = input[0].pos;
			element.col = input[0].col;

			element.pos.xyz = element.pos.xyz / g_xWorld_VoxelRadianceDataRes * 2 - 1;
			element.pos.y = -element.pos.y;
			element.pos.xyz *= g_xWorld_VoxelRadianceDataRes;
			element.pos.xyz += CreateCube(i) * 2;
			element.pos.xyz *= g_xWorld_VoxelRadianceDataRes * g_xWorld_VoxelRadianceDataSize / g_xWorld_VoxelRadianceDataRes;

			element.pos = mul(float4(element.pos.xyz, 1), g_xTransform);
			element.col *= g_xColor;

			output.Append(element);
		}
		output.RestartStrip();
	}
}