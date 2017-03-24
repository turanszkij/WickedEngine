#include "objectHF.hlsli"


struct HullInputType
{
	float3 pos								: POSITION0;
	float3 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	float3 vel								: TEXCOORD1;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};


HullInputType main(Input input)
{
	HullInputType Out = (HullInputType)0;

	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	float4 pos = input.pos;
	float4 posPrev = input.pre;
	float4 vel = float4(0,0,0,1);
		

	pos = mul( pos,WORLD );
	if(posPrev.w){
		posPrev = mul( posPrev,WORLD );
		vel = pos - posPrev;
	}


	float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
	affectWind(pos.xyz,input.tex.w,input.id);


	Out.pos=pos.xyz;
	Out.tex=input.tex.xyz;
	Out.nor.xyz = normalize(normal);
	Out.nor.w = input.nor.w;

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	return Out;
}