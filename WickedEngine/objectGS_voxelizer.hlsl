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
	GSOutput output[3];

	float3 facenormal = abs(input[0].nor + input[1].nor + input[2].nor);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	[unroll]
	for (uint i = 0; i < 3; ++i)
	{
		// World space -> Voxel grid space:
		output[i].pos.xyz = (input[i].pos.xyz - g_xFrame_VoxelRadianceDataCenter) * g_xFrame_VoxelRadianceDataSize_rcp;

		// Project onto dominant axis:
		[flatten]
		if (maxi == 0)
		{
			output[i].pos.xyz = output[i].pos.zyx;
		}
		else if (maxi == 1)
		{
			output[i].pos.xyz = output[i].pos.xzy;
		}
	}


	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(output[1].pos.xy - output[0].pos.xy);
	float2 side1N = normalize(output[2].pos.xy - output[1].pos.xy);
	float2 side2N = normalize(output[0].pos.xy - output[2].pos.xy);
	output[0].pos.xy += normalize(side2N - side0N);
	output[1].pos.xy += normalize(side0N - side1N);
	output[2].pos.xy += normalize(side1N - side2N);


	[unroll]
	for (uint j = 0; j < 3; j++)
	{
		// Voxel grid space -> Clip space
		output[j].pos.xy *= g_xFrame_VoxelRadianceDataRes_rcp;
		output[j].pos.zw = 1;

		// Append the rest of the parameters as is:
		output[j].color = input[j].color;
		output[j].uvsets = input[j].uvsets;
		output[j].N = input[j].nor;
		output[j].P = input[j].pos.xyz;

		outputStream.Append(output[j]);
	}

	outputStream.RestartStrip();
}
