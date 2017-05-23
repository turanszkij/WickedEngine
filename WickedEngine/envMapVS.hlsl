#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


VSOut main(Input_Object_ALL input)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	Out.nor = normalize(mul(normalize(input.nor.xyz), (float3x3)WORLD));
	Out.tex = input.tex.xy;
	Out.instanceColor = input.instance.color_dither.rgb;
	Out.ao = input.nor.w;

	return Out;
}