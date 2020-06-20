#include "globals.hlsli"
#include "ShaderInterop_Font.h"

TEXTURE2D(texture_font, float, TEXSLOT_FONTATLAS);
SAMPLERSTATE(sampler_font, SSLOT_ONDEMAND1);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return texture_font.SampleLevel(sampler_font, PSIn.tex, 0).rrrr * g_xFont_Color;
}
