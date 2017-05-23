#include "envMapHF.hlsli"

CBUFFER(CubemapRenderCB, CBSLOT_RENDERER_CUBEMAPRENDER)
{
	float4x4 xCubeShadowVP[6];
}

[maxvertexcount(18)]
void main(triangle VSOut input[3], inout TriangleStream<PSIn> CubeMapStream)
{
	[unroll]
	for (int f = 0; f < 6; ++f)
	{
		PSIn output;
		output.RTIndex = f;
		[unroll]
		for (uint v = 0; v < 3; v++)
		{
			output.pos = mul(input[v].pos, xCubeShadowVP[f]);
			output.pos3D = input[v].pos.xyz;
			output.tex = input[v].tex;
			output.nor = input[v].nor;
			output.instanceColor = input[v].instanceColor;
			output.ao = input[v].ao;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}