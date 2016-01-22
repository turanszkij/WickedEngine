#include "effectHF_VS.hlsli"
#include "effectHF_PSVS.hlsli"

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

	[branch]if((uint)input.tex.z == g_xMat_matIndex){
	
		float4x4 WORLD = float4x4(
				float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
				,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
				,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
				,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
			);

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