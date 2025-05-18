#include "globals.hlsli"

float4 main(float4 pos : SV_Position, float4 col : COLOR) : SV_Target
{
	return max(col, 0);
}
