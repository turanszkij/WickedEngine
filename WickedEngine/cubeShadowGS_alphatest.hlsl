#include "globals.hlsli"

struct GS_CUBEMAP_IN
{
	float4 pos		: SV_POSITION;
	float2 uv		: UV;
};
struct PS_CUBEMAP_IN
{
	float4 pos		: SV_POSITION;
	float3 pos3D	: POSITION3D;
	float2 uv		: UV;
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
			output.pos = mul(input[v].pos, xCubeShadowVP[f]);
			output.pos3D = input[v].pos.xyz;
			output.uv = input[v].uv;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}