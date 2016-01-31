#include "globals.hlsli"

Texture2D xTexture:register (t0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return xTexture.Sample(sampler_linear_clamp, PSIn.tex);
}