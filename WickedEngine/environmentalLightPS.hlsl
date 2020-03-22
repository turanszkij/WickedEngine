#include "deferredLightHF.hlsli"

TEXTURE2D(texture_ao, float, TEXSLOT_RENDERPATH_AO);
TEXTURE2D(texture_ssr, float4, TEXSLOT_RENDERPATH_SSR);


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;
	lighting.indirect.specular = max(0, EnvironmentReflection_Global(surface, envMapMIP));

	LightingContribution vxgi_contribution = VoxelGI(surface, lighting);

	float4 ssr = texture_ssr.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb, ssr.a);

	float ao = texture_ao.SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	surface.occlusion = ao;

	// Blend factor modulation:
	//	Alpha is not written, but blending will multiply the dest rgb with 1-alpha
	//	Modulate alpha to overwrite dest (which contains ambient and light map before this pass)
	ao = saturate(1 - ao);
	diffuse_alpha = max(vxgi_contribution.diffuse, ao);
	specular_alpha = max(max(vxgi_contribution.specular, ssr.a), ao);

	DEFERREDLIGHT_RETURN
}