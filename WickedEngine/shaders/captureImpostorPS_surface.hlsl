#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

float4 main(PixelInput input) : SV_Target0
{
	float4 surface_occlusion_roughness_metallic_reflectance;
	[branch]
	if (GetMaterial().uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = GetMaterial().uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surface_occlusion_roughness_metallic_reflectance = texture_surfacemap.Sample(sampler_objectshader, UV_surfaceMap);
	}
	else
	{
		surface_occlusion_roughness_metallic_reflectance = 1;
	}

	float4 surface;
	surface.r = surface_occlusion_roughness_metallic_reflectance.r;
	surface.g = GetMaterial().roughness * surface_occlusion_roughness_metallic_reflectance.g;
	surface.b = GetMaterial().metalness * surface_occlusion_roughness_metallic_reflectance.b;
	surface.a = GetMaterial().reflectance * surface_occlusion_roughness_metallic_reflectance.a;

	return surface;
}

