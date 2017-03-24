#include "skinningHF.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "sOHF.hlsli"


streamOutput main(Input_Skinning input)
{
	streamOutput Out = (streamOutput)0;

	float4 posPrev = input.pos;

#ifdef SKINNING_ON
	Skinning(input.pos,posPrev,input.nor,input.bon,input.wei);
#endif

	Out.pos=input.pos;
	Out.nor=input.nor;
	Out.pre=posPrev;

	return Out;
}