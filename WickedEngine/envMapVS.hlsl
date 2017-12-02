#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


VSOut main(Input_Object_POS_TEX input)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);
	Out.nor = normalize(mul(surface.normal, (float3x3)WORLD));
	Out.tex = surface.uv;
	Out.instanceColor = input.instance.color_dither.rgb;

	return Out;
}