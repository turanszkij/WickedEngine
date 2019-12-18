#include "envMapHF.hlsli"

[maxvertexcount(3)]
void main(triangle VSOut_EnvmapRendering input[3], inout TriangleStream<PSIn_EnvmapRendering> CubeMapStream)
{
	[unroll]
	for (uint v = 0; v < 3; v++)
	{
		PSIn_EnvmapRendering output;
		output.RTIndex = input[v].faceIndex;
		output.pos = mul(xCubeShadowVP[output.RTIndex], input[v].pos);
		output.color = input[v].color;
		output.uvsets = input[v].uvsets;
		output.atl = input[v].atl;
		output.nor = input[v].nor;
		output.pos3D = input[v].pos.xyz;
		CubeMapStream.Append(output);
	}
}