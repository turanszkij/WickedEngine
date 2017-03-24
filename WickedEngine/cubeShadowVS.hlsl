#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;    // World position 
	float2 Tex		: TEXCOORD0;      // Texture coord
};

GS_CUBEMAP_IN main(Input_Shadow input)
{
	GS_CUBEMAP_IN Out = (GS_CUBEMAP_IN)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	float4 pos = input.pos;

	Out.Pos = mul(pos, WORLD);
	Out.Tex = input.tex.xy;


	return Out;
}