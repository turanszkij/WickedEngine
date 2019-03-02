#include "globals.hlsli"
#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float  dither : DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float2 pixel = input.pos.xy;

	clip(dither(pixel) - input.dither);

	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 color = xBaseColorMap.Sample(sampler_objectshader, UV);
	color.rgb = DEGAMMA(color.rgb);
	color *= g_xMat_baseColor * float4(input.instanceColor, 1);
	ALPHATEST(color.a);
	float opacity = color.a;

	color.rgb *= 1 - opacity;

	// Use the alpha channel for refraction caustics effect:
	float3 bumpColor;
	
	bumpColor = float3(2.0f * xNormalMap.Sample(sampler_objectshader, UV - g_xMat_texMulAdd.ww).rg - 1.0f, 1);
	bumpColor.rg *= g_xMat_refractionIndex;
	bumpColor.rg *= g_xMat_normalMapStrength;
	bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));
	
	color.a = 1 - saturate(dot(bumpColor, float3(0, 0, 1)));

	return color;
}
