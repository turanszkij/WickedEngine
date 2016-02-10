#include "objectHF.hlsli"

struct HullInputType
{
	float3 pos				: POSITION0;
	float3 tex				: TEXCOORD0;
	float3 nor				: NORMAL;
	float3 vel				: TEXCOORD1;
};


HullInputType main(Input input)
{
	HullInputType Out = (HullInputType)0;

	[branch]if((uint)input.tex.z == g_xMat_matIndex)
	{
	
		float4x4 WORLD = MakeWorldMatrixFromInstance(input);

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
		Out.nor = normalize(normal);

	}

	return Out;
}