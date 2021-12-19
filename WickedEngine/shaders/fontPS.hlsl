#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	return bindless_textures[font_push.texture_index].SampleLevel(sampler_linear_clamp, input.uv, 0).rrrr * unpack_rgba(font_push.color);
}
