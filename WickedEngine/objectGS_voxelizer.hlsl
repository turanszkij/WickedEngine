#include "globals.hlsli"

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 instanceColor : COLOR;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 N : NORMAL;
	float2 tex : TEXCOORD;
	float3 P : POSITION3D;
	nointerpolation float3 instanceColor : COLOR;
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
		// voxel space pos:
		output[i].pos = float4((input[i].pos.xyz - g_xWorld_VoxelRadianceDataCenter) / g_xWorld_VoxelRadianceDataSize, 1);

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

		// projected pos:
		output[i].pos.xy /= g_xWorld_VoxelRadianceDataRes;

		output[i].pos.z = 1;

		output[i].N = input[i].nor;
		output[i].tex = input[i].tex;
		output[i].P = input[i].pos.xyz;
		output[i].instanceColor = input[i].instanceColor;
	}


	// Conservative Rasterization setup:
	float2 side0N = normalize(output[1].pos.xy - output[0].pos.xy);
	float2 side1N = normalize(output[2].pos.xy - output[1].pos.xy);
	float2 side2N = normalize(output[0].pos.xy - output[2].pos.xy);
	const float texelSize = 1.0f / g_xWorld_VoxelRadianceDataRes;
	output[0].pos.xy += normalize(-side0N + side2N)*texelSize;
	output[1].pos.xy += normalize(side0N - side1N)*texelSize;
	output[2].pos.xy += normalize(side1N - side2N)*texelSize;


	[unroll]
	for (uint j = 0; j<3; j++)
		outputStream.Append(output[j]);

	outputStream.RestartStrip();
}