#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

float4 main(float4 pos : SV_Position, half4 color : COLOR, half2 localPos : LOCALPOS) : SV_Target
{
	const float A = dot(localPos, localPos);
	const half opacity = exp(-0.5 * A) * color.a;
	return color * opacity;
}
