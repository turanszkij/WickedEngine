#include "globals.hlsli"
#include "ShaderInterop_Font.h"

Texture2D<float> texture_font:register(t1, space0);
SamplerState sampler_font:register(s2, space0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return texture_font.SampleLevel(sampler_font, PSIn.tex, 0).rrrr * fontCB.g_xFont_Color;
}
