#include "globals.hlsli"

struct GSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float3 nor : NORMAL;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float3 N : NORMAL;
	float3 P : POSITION3D;
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > outputStream
)
{
	float3 facenormal = abs(input[0].nor + input[1].nor + input[2].nor);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	for (uint i = 0; i < 3; ++i)
	{
		GSOutput output;

		// World space -> Voxel grid space:
		output.pos.xyz = (input[i].pos.xyz - g_xFrame_VoxelRadianceDataCenter) * g_xFrame_VoxelRadianceDataSize_rcp;

		// Project onto dominant axis:
		[flatten]
		if (maxi == 0)
		{
			output.pos.xyz = output.pos.zyx;
		}
		else if (maxi == 1)
		{
			output.pos.xyz = output.pos.xzy;
		}

		// Voxel grid space -> Clip space
		output.pos.xy *= g_xFrame_VoxelRadianceDataRes_rcp;
		output.pos.zw = 1;

		// Append the rest of the parameters as is:
		output.color = input[i].color;
		output.uvsets = input[i].uvsets;
		output.N = input[i].nor;
		output.P = input[i].pos.xyz;

		outputStream.Append(output);
	}
}
