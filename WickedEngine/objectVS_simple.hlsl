#include "objectHF.hlsli"



PixelInputType_Simple main(Input_Simple input)
{
	PixelInputType_Simple Out = (PixelInputType_Simple)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	float4 pos = input.pos;

	pos = mul(pos, WORLD);

	Out.clip = dot(pos, g_xClipPlane);

	affectWind(pos.xyz, input.tex.w, input.id, g_xFrame_Time);


	Out.pos = mul(pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}