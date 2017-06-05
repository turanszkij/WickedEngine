#include "objectHF.hlsli"

PixelInputType main(Input_Object_ALL input)
{
	PixelInputType Out = (PixelInputType)0;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
		
	float4 pos = float4(input.pos.xyz, 1);
	pos = mul(pos, WORLD);
		
	Out.pos = Out.pos2D = mul(pos, g_xCamera_VP);
	Out.pos3D = pos.xyz;
	Out.tex = input.tex.xy;
	Out.nor = mul(input.nor.xyz, (float3x3)WORLD);
	Out.nor = normalize(Out.nor);
	Out.nor2D = mul(Out.nor.xyz, (float3x3)g_xCamera_VP).xy;


	Out.ReflectionMapSamplingPos = mul(pos, g_xFrame_MainCamera_ReflVP);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	Out.ao = 1;

	return Out;
}