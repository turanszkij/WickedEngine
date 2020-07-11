#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


PSIn_EnvmapRendering main(Input_Object_ALL input)
{
	PSIn_EnvmapRendering output;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	uint frustum_index = input.inst.userdata.y;
	output.RTIndex = xCubemapRenderCams[frustum_index].properties.x;
	output.pos = mul(WORLD, surface.position);
	output.pos3D = output.pos.xyz;
	output.pos = mul(xCubemapRenderCams[frustum_index].VP, output.pos);
	output.color = surface.color;
	output.uvsets = surface.uvsets;
	output.atl = surface.atlas;
	output.nor = normalize(mul((float3x3)WORLD, surface.normal));

	return output;
}
