//#include "skinningHF.hlsli"
#include "effectInputLayoutHF.hlsli"

cbuffer matBuffer:register(b1){
	float4 diffuseColor;
	float4 hasRefNorTexSpe;
	float4 specular;
	float4 refractionIndexMovingTexEnv;
	uint shadeless;
	uint specular_power;
	uint toonshaded;
	uint matIndex;
};

struct GS_CUBEMAP_IN 
{ 
	float4 Pos		: SV_POSITION;    // World position 
    float2 Tex		: TEXCOORD0;         // Texture coord
}; 

GS_CUBEMAP_IN main(Input input)
{
	GS_CUBEMAP_IN Out = (GS_CUBEMAP_IN)0;

	if((uint)input.tex.z == matIndex){
		
		float4x4 WORLD = float4x4(
				float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
				,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
				,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
				,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
			);

		float4 pos = input.pos;
		
//#ifdef SKINNING_ON
//		Skinning(pos,input.bon,input.wei);
//#endif

		Out.Pos = mul(pos,WORLD);
		Out.Tex = input.tex.xy;

	}

	return Out;
}