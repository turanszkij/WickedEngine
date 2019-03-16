#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct GS_CUBEMAP_IN
{
	float4 pos : SV_POSITION;    // World position 
	float2 uv : UV;
};

GS_CUBEMAP_IN main(Input_Object_POS_TEX input)
{
	GS_CUBEMAP_IN Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);
	Out.uv = g_xMat_uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return Out;
}