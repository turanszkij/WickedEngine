#include "globals.hlsli"

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 P : POSITION3D;
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > output
)
{
	GSOutput element;

	float3 facenormal = abs(normalize(input[0].nor + input[1].nor + input[2].nor));
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	[unroll]
	for (uint i = 0; i < 3; ++i)
	{
		// voxel space pos:
		element.pos = float4(input[i].pos.xyz / g_xWorld_VoxelRadianceDataSize - g_xWorld_VoxelRadianceDataCenter, 1);

		if (maxi == 0)
		{
			element.pos.xyz = element.pos.zyx;
		}
		else if (maxi == 1)
		{
			element.pos.xyz = element.pos.xzy;
		}

		// projected pos:
		element.pos.xy /= g_xWorld_VoxelRadianceDataRes;

		element.pos.z = 1;

		element.nor = input[i].nor;
		element.tex = input[i].tex;
		element.P = input[i].pos.xyz;
		output.Append(element);
	}
}