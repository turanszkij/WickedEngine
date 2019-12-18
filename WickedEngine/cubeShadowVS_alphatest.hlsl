#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"

struct VertexOut
{
	float4 pos		: SV_POSITION;
	float2 uv		: UV;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut output;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	output.RTIndex = input.inst.userdata.y;
	output.pos = mul(xCubeShadowVP[output.RTIndex], mul(WORLD, surface.position));
	output.uv = g_xMaterial.uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return output;
}
