#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


VSOut_EnvmapRendering main(Input_Object_ALL input)
{
	VSOut_EnvmapRendering Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(WORLD, surface.position);
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.atl = surface.atlas;
	Out.nor = normalize(mul((float3x3)WORLD, surface.normal));

	return Out;
}