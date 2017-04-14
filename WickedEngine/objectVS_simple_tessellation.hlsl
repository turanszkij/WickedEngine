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


HullInputType main(Input_Simple input)
{
	HullInputType Out = (HullInputType)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	float4 pos = input.pos;


	pos = mul(pos, WORLD);
	affectWind(pos.xyz, input.tex.w, input.id, g_xFrame_Time);


	Out.pos = pos.xyz;
	Out.tex = input.tex.xyz;

	// todo: leave these but I'm lazy to create appropriate hull/domain shaders now...
	Out.posPrev = 0;
	Out.nor = 0;
	Out.instanceColor = 0;
	Out.dither = 0;

	return Out;
}