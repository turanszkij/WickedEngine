#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float4 surface_occlusion_roughness_metallic_reflectance;
	[branch]
	if (g_xMaterial.uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = g_xMaterial.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surface_occlusion_roughness_metallic_reflectance = texture_surfacemap.Sample(sampler_objectshader, UV_surfaceMap);

		if (g_xMaterial.IsUsingSpecularGlossinessWorkflow())
		{
			ConvertToSpecularGlossiness(surface_occlusion_roughness_metallic_reflectance);
		}
	}
	else
	{
		surface_occlusion_roughness_metallic_reflectance = 1;
	}

	float4 surface;
	surface.r = surface_occlusion_roughness_metallic_reflectance.r;
	surface.g = g_xMaterial.roughness * surface_occlusion_roughness_metallic_reflectance.g;
	surface.b = g_xMaterial.metalness * surface_occlusion_roughness_metallic_reflectance.b;
	surface.a = g_xMaterial.reflectance * surface_occlusion_roughness_metallic_reflectance.a;

	return surface;
}

