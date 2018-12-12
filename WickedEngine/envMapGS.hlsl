#include "envMapHF.hlsli"

[maxvertexcount(18)]
void main(triangle VSOut_EnvmapRendering input[3], inout TriangleStream<PSIn_EnvmapRendering> CubeMapStream)
{
	[unroll]
	for (int f = 0; f < 6; ++f)
	{
		PSIn_EnvmapRendering output;
		output.RTIndex = f;
		[unroll]
		for (uint v = 0; v < 3; v++)
		{
			output.pos = mul(input[v].pos, xCubeShadowVP[f]);
			output.pos3D = input[v].pos.xyz;
			output.tex = input[v].tex;
			output.atl = input[v].atl;
			output.nor = input[v].nor;
			output.instanceColor = input[v].instanceColor;
			output.ao = input[v].ao;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}