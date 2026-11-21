#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

float4 main(PSIn input) : SV_TARGET
{
	if (ocean_exclusion_check(input.GetPos3D()))
	{
		discard;
	}
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
