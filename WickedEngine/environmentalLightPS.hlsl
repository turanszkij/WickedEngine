#include "deferredLightHF.hlsli"

#define	xSSR texture_9


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;
	specular = max(0, EnvironmentReflection_Global(surface, envMapMIP)) * surface.F;
	specular_alpha = 1;

	VoxelGIResult vxgiresult = VoxelGI(surface);
	diffuse = vxgiresult.diffuse.rgb;
	diffuse_alpha = vxgiresult.diffuse.a;
	specular = lerp(specular, vxgiresult.specular.rgb, vxgiresult.specular.a);

	float4 ssr = xSSR.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	specular = lerp(specular, ssr.rgb * surface.F, ssr.a);

	specular_alpha = max(vxgiresult.specular.a, ssr.a);

	DEFERREDLIGHT_RETURN
}