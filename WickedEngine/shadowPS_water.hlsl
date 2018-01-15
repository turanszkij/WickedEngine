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

	OBJECT_PS_SAMPLETEXTURES_SIMPLE

	OBJECT_PS_DEGAMMA

	// Water does not generate shadows to not mess up the smooth falloff on geometry intersections:
	//  This could be resolved if this shader could sample depth, but the depthbuffer is currently bound
	//  so leave it at this for the moment
	color.rgb = 1;

	// Use the alpha channel for refraction caustics effect:
	float3 bumpColor;

	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	bumpColor0 = 2.0f * xNormalMap.Sample(sampler_objectshader, UV - g_xMat_texMulAdd.ww).rg - 1.0f;
	bumpColor1 = 2.0f * xNormalMap.Sample(sampler_objectshader, UV + g_xMat_texMulAdd.zw).rg - 1.0f;
	bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * g_xMat_refractionIndex;
	bumpColor.rg *= g_xMat_normalMapStrength;
	bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));

	color.a = 1 - saturate(abs(dot(bumpColor, float3(0, 0, 1))));

	OBJECT_PS_OUT_FORWARD_SIMPLE
}
