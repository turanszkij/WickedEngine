#include "globals.hlsli"

float4 main(float4 pos : SV_POSITION) : SV_TARGET
{
	return g_xColor;
}