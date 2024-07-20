#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

struct ImpostorOutput
{
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
	float4 surface : SV_Target2;
};

ImpostorOutput main(PixelInput input)
{
	ShaderMaterial material = GetMaterial();

	float4 uvsets = input.GetUVSets();
	
	float4 color;
	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		color = material.textures[BASECOLORMAP].Sample(sampler_linear_wrap, uvsets);
	}
	else
	{
		color = 1;
	}
	color *= input.color;
	clip(color.a - material.GetAlphaTest());

	float3 N = normalize(input.nor);
	float3 P = input.pos3D;

	float3x3 TBN = compute_tangent_frame(N, P, uvsets.xy);

	[branch]
	if (material.textures[NORMALMAP].IsValid())
	{
		[branch]
		if (material.GetNormalMapStrength() > 0 && material.textures[NORMALMAP].IsValid())
		{
			float3 normalMap = float3(material.textures[NORMALMAP].Sample(sampler_objectshader, uvsets).rg, 1);
			float3 bumpColor = normalMap.rgb * 2 - 1;
			N = normalize(lerp(N, mul(bumpColor, TBN), material.GetNormalMapStrength()));
		}
	}

	float4 surfaceMap = 1;
	[branch]
	if (material.textures[SURFACEMAP].IsValid())
	{
		surfaceMap = material.textures[SURFACEMAP].Sample(sampler_linear_wrap, uvsets);
	}

	float4 surface;
	surface.r = surfaceMap.r;
	surface.g = material.GetRoughness() * surfaceMap.g;
	surface.b = material.GetMetalness() * surfaceMap.b;
	surface.a = material.GetReflectance() * surfaceMap.a;

	ImpostorOutput output;
	output.color = color;
	output.normal = float4(N * 0.5f + 0.5f, 1);
	output.surface = surface;
	return output;
}

