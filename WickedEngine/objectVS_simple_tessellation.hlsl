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

	float4 pos = float4(input.pos.xyz, 1);


	pos = mul(pos, WORLD);
	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);

	float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);

	Out.pos = pos.xyz;
	Out.tex = input.tex.xyz;

	// note: simple vs doesn't have normal but this needs it because tessellation is using normal information
	Out.nor = float4(normalize(normal), input.nor.w);

	// todo: leave these but I'm lazy to create appropriate hull/domain shaders now...
	Out.posPrev = 0;
	Out.instanceColor = 0;
	Out.dither = 0;

	return Out;
}