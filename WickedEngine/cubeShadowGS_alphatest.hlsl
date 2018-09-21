#include "globals.hlsli"

struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;
	float2 Tex		: TEXCOORD0;
};
struct PS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;
	float3 pos3D	: POSITION3D;
	float2 Tex		: TEXCOORD0;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle GS_CUBEMAP_IN input[3], inout TriangleStream<PS_CUBEMAP_IN> CubeMapStream)
{
	[unroll]
	for (int f = 0; f < 6; ++f)
	{
		PS_CUBEMAP_IN output;
		output.RTIndex = f;
		[unroll]
		for (int v = 0; v < 3; v++)
		{
			output.Pos = mul(input[v].Pos, xCubeShadowVP[f]);
			output.pos3D = input[v].Pos.xyz;
			output.Tex = input[v].Tex;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}