#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<GaussianSplat> splats : register(t0);

float4 main(float4 pos : SV_Position, float2 localPos : LOCALPOS, float3 color : COLOR, uint splatID : SPLATID) : SV_Target
{
	const float A = dot(localPos, localPos);
	const half opacity = exp(-0.5 * A) * splats[splatID].opacity;
	return float4(color * opacity, opacity);
}
