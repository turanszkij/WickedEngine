#include "globals.hlsli"
#include "hairparticleHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}