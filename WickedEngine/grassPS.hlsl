struct GS_OUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float3 col : COLOR;
};
#include "grassHF_PS.hlsli"

PS_OUT main(GS_OUT PSIn)
{
	PS_OUT Out = (PS_OUT)0;
	Out.col = float4(PSIn.col,1);
	Out.nor = float4(PSIn.nor,0);
	//Out.spe = float4(PSIn.col,0.1f);
	//Out.mat = float4(50,0,0,0);
	return Out;
}