//#include "skinningHF.hlsli"
#include "windHF.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "globals.hlsli"


struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertextoPixel main(Input input)
{
	VertextoPixel Out = (VertextoPixel)0;

	float4x4 WORLD = float4x4(
			float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
			,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
			,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
			,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
		);

	float4 pos = input.pos;
		
	pos=mul(pos,WORLD);
	affectWind(pos.xyz,input.tex.w,input.id);

	Out.pos = mul( pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}