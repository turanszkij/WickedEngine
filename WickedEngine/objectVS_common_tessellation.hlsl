#include "objectHF.hlsli"


struct HullInputType
{
	float3 pos								: POSITION;
	float3 posPrev							: POSITIONPREV;
	float2 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};


HullInputType main(Input_Object_ALL input)
{
	HullInputType Out;

	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instancePrev);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);
		

	surface.position = mul(surface.position, WORLD);
	surface.prevPos = mul(surface.prevPos, WORLDPREV);
	surface.normal = normalize(mul(surface.normal, (float3x3)WORLD));


	affectWind(surface.position.xyz, surface.wind, g_xFrame_Time);
	affectWind(surface.prevPos.xyz, surface.wind, g_xFrame_TimePrev);


	Out.pos = surface.position.xyz;
	Out.posPrev = surface.prevPos.xyz;
	Out.tex = surface.uv;
	Out.nor = float4(surface.normal, 1);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	return Out;
}