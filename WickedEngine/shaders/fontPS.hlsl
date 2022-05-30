#include "globals.hlsli"
#include "ShaderInterop_Font.h"

struct VertextoPixel
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float dist = bindless_textures[font_push.texture_index].SampleLevel(sampler_linear_clamp, input.uv, 0).r;
	float4 color = smoothstep(0.6, 0.75, dist) * unpack_rgba(font_push.color);
	// outline:
	color += smoothstep(0.1, 0.6, dist) * float4(0, 0, 0, 1);
	return color;
}
