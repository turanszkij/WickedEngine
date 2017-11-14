#include "globals.hlsli"

struct GSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

float4 main(GSOut input) : SV_TARGET
{
	return input.color;
}
