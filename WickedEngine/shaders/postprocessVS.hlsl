#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

struct Output
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD;
};

Output main(uint vI : SV_VERTEXID)
{
	Output Out;

	vertexID_create_fullscreen_triangle(vI, Out.pos, Out.uv);

	return Out;
}
