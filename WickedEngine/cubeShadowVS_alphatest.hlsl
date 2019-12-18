#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct GS_CUBEMAP_IN
{
	float4 pos : SV_POSITION;    // World position
	uint faceIndex	: FACEINDEX;
	float2 uv : UV;
};

GS_CUBEMAP_IN main(Input_Object_POS_TEX input)
{
	GS_CUBEMAP_IN Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(WORLD, surface.position);
	Out.faceIndex = input.inst.userdata.y;
	Out.uv = g_xMaterial.uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return Out;
}