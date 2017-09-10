#include "objectHF.hlsli"


struct HullInputType
{
	float3 pos								: POSITION;
	float3 posPrev							: POSITIONPREV;
	float3 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};


HullInputType main(Input_Object_ALL input)
{
	HullInputType Out = (HullInputType)0;

	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instancePrev);

	float4 pos = float4(input.pos.xyz, 1);
	float4 posPrev = float4(input.pre.xyz, 1);
		

	pos = mul(pos, WORLD);
	posPrev = mul(posPrev, WORLDPREV);


	float3 normal = mul(normalize(input.nor.xyz * 2 - 1), (float3x3)WORLD);
	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);
	affectWind(posPrev.xyz,input.pos.w, g_xFrame_TimePrev);


	Out.pos=pos.xyz;
	Out.posPrev = posPrev.xyz;
	Out.tex=input.tex.xyz;
	Out.nor = float4(normalize(normal), input.nor.w);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	return Out;
}