#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


struct VertexOut
{
	float4 pos	: SV_POSITION;
};

VertexOut main(Input_Object_POS input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);
		
	Out.pos = mul(surface.position, WORLD);
	affectWind(Out.pos.xyz, surface.wind, g_xFrame_Time);

	Out.pos = mul(Out.pos, g_xCamera_VP);

	return Out;
}