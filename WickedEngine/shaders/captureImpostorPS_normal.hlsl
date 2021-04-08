#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

float4 main(PixelInput input) : SV_Target0
{
	float3 N = normalize(input.nor);
	float3 P = input.pos3D;

	float3x3 TBN = compute_tangent_frame(N, P, input.uvsets.xy);

	[branch]
	if (GetMaterial().uvset_normalMap >= 0)
	{
		float3 bumpColor;
		NormalMapping(input.uvsets, N, TBN, bumpColor);
	}

	return float4(N * 0.5f + 0.5f, 1);
}

