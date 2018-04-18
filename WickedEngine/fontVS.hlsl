#include "globals.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertextoPixel main(float2 inPos : POSITION, float2 inTex : TEXCOORD0)
{
	VertextoPixel Out = (VertextoPixel)0;

	Out.pos = mul(float4(inPos, 0, 1), g_xTransform);
	
	Out.tex=inTex;

#ifdef SHADERCOMPILER_SPIRV
	Out.pos.y = -Out.pos.y;
#endif 

	return Out;
}
