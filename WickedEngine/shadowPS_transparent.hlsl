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
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE_SIMPLE

	// When opacity reaches ZERO, the multiplicative light mask will be ONE:
	color.rgb = lerp(1, color.rgb, opacity);

	// Use the alpha channel for refraction caustics effect:
	float3 bumpColor;
	
	bumpColor = float3(2.0f * xNormalMap.Sample(sampler_objectshader, UV - g_xMat_texMulAdd.ww).rg - 1.0f, 1);
	bumpColor.rg *= g_xMat_refractionIndex;
	bumpColor.rg *= g_xMat_normalMapStrength;
	bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));
	
	color.a = 1 - saturate(dot(bumpColor, float3(0, 0, 1)));

	OBJECT_PS_OUT_FORWARD_SIMPLE
}
