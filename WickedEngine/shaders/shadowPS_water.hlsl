#define OBJECTSHADER_LAYOUT_POS_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	float2 pixel = input.pos.xy;

	float4 color;
	[branch]
	if (GetMaterial().uvset_baseColorMap >= 0)
	{
		const float2 UV_baseColorMap = GetMaterial().uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
	}
	else
	{
		color = 1;
	}
	color *= input.color;

	float opacity = color.a;

	color.rgb = 1; // disable water shadow because it has already fog

	[branch]
	if (GetMaterial().uvset_normalMap >= 0)
	{
		float3 bumpColor;

		float2 bumpColor0 = 0;
		float2 bumpColor1 = 0;
		float2 bumpColor2 = 0;
		const float2 UV_normalMap = GetMaterial().uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		bumpColor0 = 2.0f * texture_normalmap.Sample(sampler_objectshader, UV_normalMap - GetMaterial().texMulAdd.ww).rg - 1.0f;
		bumpColor1 = 2.0f * texture_normalmap.Sample(sampler_objectshader, UV_normalMap + GetMaterial().texMulAdd.zw).rg - 1.0f;
		bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * GetMaterial().refraction;
		bumpColor.rg *= GetMaterial().normalMapStrength;
		bumpColor = normalize(max(bumpColor, float3(0, 0, 0.0001f)));

		float caustic = abs(dot(bumpColor, float3(0, 1, 0)));
		caustic = saturate(caustic);
		caustic = pow(caustic, 2);
		caustic *= 20;

		color.rgb += caustic;
	}

	color.a = input.pos.z; // secondary depth

	return color;
}
