#include "globals.hlsli"
#include "hairparticleHF.hlsli"

float4 main() : SV_TARGET
{
	return float4(xColor.rgb, 1.0f);
}