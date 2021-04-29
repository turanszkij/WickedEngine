#include "imageHF.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	return texture_base.SampleLevel(Sampler, uv, 0) * xColor;
}