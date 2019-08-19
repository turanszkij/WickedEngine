#include "globals.hlsli"
#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float4 color			: COLOR;
	float4 uvsets			: UVSETS;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float2 pixel = input.pos.xy;

	float4 color;
	[branch]
	if (g_xMaterial.uvset_baseColorMap >= 0)
	{
		const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
	}
	else
	{
		color = 1;
	}
	color *= input.color;
	ALPHATEST(color.a);
	float opacity = color.a;

	color.rgb *= 1 - opacity;

	// Use the alpha channel for refraction caustics effect:
	[branch]
	if (g_xMaterial.uvset_normalMap >= 0)
	{
		float3 bumpColor;

		const float2 UV_normalMap = g_xMaterial.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		bumpColor = float3(2.0f * texture_normalmap.Sample(sampler_objectshader, UV_normalMap - g_xMaterial.texMulAdd.ww).rg - 1.0f, 1);
		bumpColor.rg *= g_xMaterial.refractionIndex;
		bumpColor.rg *= g_xMaterial.normalMapStrength;
		bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));

		color.a = 1 - saturate(dot(bumpColor, float3(0, 0, 1)));
	}

	return color;
}
