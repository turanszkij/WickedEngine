#include "globals.hlsli"

CBUFFER(voxelCamBuffer, 0)
{
	float4x4 xVoxelCam[SCENE_VOXELIZATION_RESOLUTION];
}

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
	uint RTIndex	: SV_RenderTargetArrayIndex;
};

[maxvertexcount(3 * SCENE_VOXELIZATION_RESOLUTION)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > output
)
{
	[unroll]
	for (uint i = 0; i < SCENE_VOXELIZATION_RESOLUTION; i++)
	{
		GSOutput element;
		element.RTIndex = i;

		[unroll]
		for (uint j = 0; j < 3; ++j)
		{
			element.pos = mul(float4(input[j].pos.xyz, 1), xVoxelCam[i]);
			element.nor = input[j].nor;
			element.tex = input[j].tex;
			output.Append(element);
		}
		output.RestartStrip();
	}
}