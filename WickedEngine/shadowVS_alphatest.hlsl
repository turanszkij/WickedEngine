#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 uv				: UV;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(WORLD, surface.position);

	Out.pos = mul(g_xCamera_VP, Out.pos);
	Out.uv = g_xMaterial.uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return Out;
}