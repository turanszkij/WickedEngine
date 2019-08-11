#include "imageHF.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	return xTexture.SampleLevel(Sampler, uv, xMipLevel) * xColor;
}