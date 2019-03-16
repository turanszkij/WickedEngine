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
			output.color = input[v].color;
			output.uvsets = input[v].uvsets;
			output.atl = input[v].atl;
			output.nor = input[v].nor;
			output.pos3D = input[v].pos.xyz;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}