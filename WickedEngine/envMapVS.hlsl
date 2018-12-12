#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


VSOut_EnvmapRendering main(Input_Object_ALL input)
{
	VSOut_EnvmapRendering Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);
	Out.nor = normalize(mul(surface.normal, (float3x3)WORLD));
	Out.tex = surface.uv;
	Out.atl = surface.atlas;
	Out.instanceColor = input.instance.color_dither.rgb;
	Out.ao = 1;

	return Out;
}