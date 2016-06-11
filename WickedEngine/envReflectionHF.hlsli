#ifndef _ENVIRONMENTMAPPING_HF_
#define _ENVIRONMENTMAPPING_HF_

// #define ENVMAP_CAMERA_BLEND
inline float3 EnvironmentReflection(in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	uint mip0 = 0;
	float2 size0;
	float mipLevels0;
	texture_env0.GetDimensions(mip0, size0.x, size0.y, mipLevels0);
	uint mip1 = 0;
	float2 size1;
	float mipLevels1;
	texture_env1.GetDimensions(mip1, size1.x, size1.y, mipLevels1);
	float3 ref = -reflect(V, N);
	float4 envCol0 = texture_env0.SampleLevel(sampler_linear_clamp, ref, roughness*mipLevels0);
	float4 envCol1 = texture_env1.SampleLevel(sampler_linear_clamp, ref, roughness*mipLevels0);
#ifdef ENVMAP_CAMERA_BLEND
	float dist0 = distance(g_xCamera_CamPos, g_xTransform[0].xyz);
	float dist1 = distance(g_xCamera_CamPos, g_xTransform[1].xyz);
#else
	float dist0 = distance(P, g_xTransform[0].xyz);
	float dist1 = distance(P, g_xTransform[1].xyz);
#endif
	static const float blendStrength = 0.05f;
	float blend = clamp((dist0 - dist1)*blendStrength, -1, 1)*0.5f + 0.5f;
	float4 envCol = lerp(envCol0, envCol1, blend);

	float3 F = F_Fresnel(f0, abs(dot(N, V)) + 1e-5f);
	
	return max(envCol.rgb * F, 0);
}

#endif // _ENVIRONMENTMAPPING_HF_