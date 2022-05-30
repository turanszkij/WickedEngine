#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float dist = bindless_textures[font.texture_index].SampleLevel(sampler_linear_clamp, input.uv, 0).r;
	float4 color = smoothstep(font.sdf_threshold_bottom, font.sdf_threshold_top, dist) * unpack_rgba(font.color);
	return color;
}
