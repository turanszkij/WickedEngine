#include "deferredLightHF.hlsli"
#include "envReflectionHF.hlsli"



LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	diffuse = 0;
	float envMapMIP = roughness * g_xWorld_EnvProbeMipCount;
	float3 R = -reflect(V, N);
	float f90 = saturate(50.0 * dot(f0, 0.33));
	float3 F = F_Schlick(f0, f90, abs(dot(N, V)) + 1e-5f);
	specular = max(0, texture_env_global.SampleLevel(sampler_linear_clamp, R, envMapMIP).rgb * F);
	VoxelRadiance(N, V, P, f0, roughness, diffuse, specular, ao);

	DEFERREDLIGHT_RETURN
}