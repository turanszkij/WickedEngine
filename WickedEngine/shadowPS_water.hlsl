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

#ifndef DISABLE_ALPHATEST
	clip(color.a - g_xMaterial.alphaTest);
#endif // DISABLE_ALPHATEST

	float opacity = color.a;

	color.rgb = 1; // disable water shadow because it has already fog

	[branch]
	if (g_xMaterial.uvset_normalMap >= 0)
	{
		float3 bumpColor;

		float2 bumpColor0 = 0;
		float2 bumpColor1 = 0;
		float2 bumpColor2 = 0;
		const float2 UV_normalMap = g_xMaterial.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		bumpColor0 = 2.0f * texture_normalmap.Sample(sampler_objectshader, UV_normalMap - g_xMaterial.texMulAdd.ww).rg - 1.0f;
		bumpColor1 = 2.0f * texture_normalmap.Sample(sampler_objectshader, UV_normalMap + g_xMaterial.texMulAdd.zw).rg - 1.0f;
		bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * g_xMaterial.refractionIndex;
		bumpColor.rg *= g_xMaterial.normalMapStrength;
		bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));

		float caustic = abs(dot(bumpColor, float3(0, 1, 0)));
		caustic = saturate(caustic);
		caustic = pow(caustic, 2);
		caustic *= 20;

		color.rgb += caustic;
	}

	return color;
}
