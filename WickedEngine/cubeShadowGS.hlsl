#include "globals.hlsli"

struct GS_CUBEMAP_IN 
{ 
	float4 pos		: SV_POSITION;
	uint faceIndex	: FACEINDEX;
}; 
struct PS_CUBEMAP_IN
{
	float4 pos		: SV_POSITION;
    uint RTIndex	: SV_RenderTargetArrayIndex; 
};


[maxvertexcount(3)] 
void main( triangle GS_CUBEMAP_IN input[3], inout TriangleStream<PS_CUBEMAP_IN> CubeMapStream ) 
{
	[unroll]
	for (int v = 0; v < 3; v++)
	{
		PS_CUBEMAP_IN output;
		output.RTIndex = input[v].faceIndex;
		output.pos = mul(xCubeShadowVP[output.RTIndex], input[v].pos);
		CubeMapStream.Append(output);
	}
}