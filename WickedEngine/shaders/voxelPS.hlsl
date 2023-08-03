#include "globals.hlsli"

float4 main(float4 pos : SV_Position, float4 col : TEXCOORD) : SV_Target
{
	return col;
}
