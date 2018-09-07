#include "objectHF.hlsli"

PixelInputType_Simple main(Input_Object_POS_TEX input)
{
	PixelInputType_Simple Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	surface.position = mul(surface.position, WORLD);

	Out.clip = dot(surface.position, g_xClipPlane);

	Out.pos = mul(surface.position, g_xCamera_VP);
	Out.tex = surface.uv;

	return Out;
}