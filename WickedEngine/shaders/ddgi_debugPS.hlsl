#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

struct VSOut
{
	float4 pos : SV_Position;
	float3 normal : NORMAL;
	uint probeIndex : PROBEINDEX;
};

float4 main(VSOut input) : SV_Target
{
	Texture2D ddgiColorTexture = bindless_textures[descriptor_index(GetScene().ddgi.color_texture)];

	uint3 probeCoord = ddgi_probe_coord(input.probeIndex);
	float3 color = ddgiColorTexture.SampleLevel(sampler_linear_clamp, ddgi_probe_color_uv(probeCoord, input.normal), 0).rgb;

	return float4(color, 1);
}
