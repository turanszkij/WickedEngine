#include "globals.hlsli"

float4 main(float4 pos : SV_Position, half4 col : COLOR) : SV_Target
{
	return col;
}
