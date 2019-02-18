#include "objectHF.hlsli"

struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float  dither : DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.instanceColor = input.inst.color_dither.rgb;
	Out.dither = input.inst.color_dither.a;

	surface.position = mul(surface.position, WORLD);

	Out.pos = mul(surface.position, g_xCamera_VP);
	Out.tex = surface.uv;

	return Out;
}