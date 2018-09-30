#include "envMapHF.hlsli"

[maxvertexcount(18)]
void main(triangle VSOut_Sky_EnvmapRendering input[3], inout TriangleStream< PSIn_Sky_EnvmapRendering > CubeMapStream)
{
	for (int f = 0; f < 6; ++f)
	{
		PSIn_Sky_EnvmapRendering output;
		output.RTIndex = f;
		for (uint v = 0; v < 3; v++)
		{
			output.pos = mul(input[v].pos, xCubeShadowVP[f]);
			output.nor = input[v].nor;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}