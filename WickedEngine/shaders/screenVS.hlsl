#include "globals.hlsli"

struct Output
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD;
};

Output main(uint vI : SV_VERTEXID)
{
	Output Out;

	FullScreenTriangle(vI, Out.pos, Out.uv);

	return Out;
}
