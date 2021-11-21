#include "globals.hlsli"
#include "ShaderInterop_Font.h"

SamplerState sampler_font : register(s0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return bindless_textures[push.texture_index].SampleLevel(sampler_font, PSIn.tex, 0).rrrr * unpack_rgba(push.color);
}
