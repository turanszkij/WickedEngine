#include "globals.hlsli"
#include "impostorHF.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	float4 color = unpack_rgba(input.instanceColor);
	return color;
}
