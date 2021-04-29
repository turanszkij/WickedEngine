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

			element.pos.xyz = element.pos.xyz / g_xFrame_VoxelRadianceDataRes * 2 - 1;
			element.pos.y = -element.pos.y;
			element.pos.xyz *= g_xFrame_VoxelRadianceDataRes;
			element.pos.xyz += (CreateCube(i) - float3(0, 1, 0)) * 2;
			element.pos.xyz *= g_xFrame_VoxelRadianceDataRes * g_xFrame_VoxelRadianceDataSize / g_xFrame_VoxelRadianceDataRes;

			element.pos = mul(g_xTransform, float4(element.pos.xyz, 1));
			element.col *= g_xColor;

			output.Append(element);
		}
		output.RestartStrip();
	}
}