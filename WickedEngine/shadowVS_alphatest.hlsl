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

	Out.pos = mul(surface.position, WORLD);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.uv = g_xMat_uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return Out;
}