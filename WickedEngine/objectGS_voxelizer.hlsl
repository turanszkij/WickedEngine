#include "globals.hlsli"

CBUFFER(voxelCamBuffer, CBSLOT_RENDERER_VOXELIZER)
{
	float4x4 xVoxelCam[SCENE_VOXELIZATION_RESOLUTION];
}

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	uint RTIndex : TEXCOORD1;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > output
)
{
	GSOutput element;
	element.RTIndex = input[0].RTIndex;

	[unroll]
	for (uint i = 0; i < 3; ++i)
	{
		element.pos = mul(float4(input[i].pos.xyz, 1), xVoxelCam[ element.RTIndex ]);
		element.nor = input[i].nor;
		element.tex = input[i].tex;
		output.Append(element);
	}
}