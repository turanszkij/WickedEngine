#include "skinningHF.hlsli"
#include "sOHF.hlsli"

struct Input{
	float4 pos : POSITION;
	float3 nor : NORMAL;
	float4 tex : TEXCOORD0;
	float4 bon : TEXCOORD1;
	float4 wei : TEXCOORD2;
};


streamOutput main( Input input )
{
	streamOutput Out = (streamOutput)0;

	float4 posPrev = input.pos;
#ifdef SKINNING_ON
	Skinning(input.pos,posPrev,input.nor,input.bon,input.wei);
#endif

	Out.pos=input.pos;
	Out.nor=input.nor;
	Out.tex=input.tex;
	Out.pre=posPrev;
	//Out.bon=input.bon;
	//Out.wei=input.wei;

	return Out;
}