#include "globals.hlsli"

struct GSOutput
{
	float4 pos : SV_Position;
	float4 col : COLOR;
};

float4 main(GSOutput PSIn) : SV_Target
{
	return float4(PSIn.col.rgb, 1);
}
