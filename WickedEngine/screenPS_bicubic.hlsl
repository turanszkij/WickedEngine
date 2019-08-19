#include "imageHF.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	return SampleTextureCatmullRom(texture_base, uv, xMipLevel) * xColor;
}