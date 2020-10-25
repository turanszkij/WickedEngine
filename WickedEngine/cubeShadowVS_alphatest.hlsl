#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"

struct VertexOut
{
	float4 pos		: SV_POSITION;
	float2 uv		: UV;
#ifdef VPRT_EMULATION
	uint RTIndex	: RTINDEX;
#else
	uint RTIndex	: SV_RenderTargetArrayIndex;
#endif // VPRT_EMULATION
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut output;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	uint frustum_index = input.inst.userdata.y;
	output.RTIndex = xCubemapRenderCams[frustum_index].properties.x;
	output.pos = mul(xCubemapRenderCams[frustum_index].VP, mul(WORLD, surface.position));
	output.uv = g_xMaterial.uvset_baseColorMap == 0 ? surface.uvsets.xy : surface.uvsets.zw;

	return output;
}
