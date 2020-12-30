#ifndef WI_LIGHTING_HF
#define WI_LIGHTING_HF
#include "globals.hlsli"
#include "brdf.hlsli"
#include "voxelConeTracingHF.hlsli"
#include "skyHF.hlsli"

#ifdef BRDF_CARTOON
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_SOFT_RTSHADOW
#endif // BRDF_CARTOON

struct LightingPart
{
	float3 diffuse;
	float3 specular;
};
struct Lighting
{
	LightingPart direct;
	LightingPart indirect;

	inline void create(
		in float3 diffuse_direct,
		in float3 specular_direct,
		in float3 diffuse_indirect,
		in float3 specular_indirect
	)
	{
		direct.diffuse = diffuse_direct;
		direct.specular = specular_direct;
		indirect.diffuse = diffuse_indirect;
		indirect.specular = specular_indirect;
	}
};

// Combine the direct and indirect lighting into final contribution
inline LightingPart CombineLighting(in Surface surface, in Lighting lighting)
{
	LightingPart result;
	result.diffuse = lighting.direct.diffuse + lighting.indirect.diffuse * surface.occlusion;
	result.specular = lighting.direct.specular + lighting.indirect.specular * surface.F * surface.occlusion;

	return result;
}

inline float3 shadowCascade(in ShaderEntity light, in float3 shadowPos, in float2 shadowUV, in uint cascade)
{
	const float slice = light.GetTextureIndex() + cascade;
	const float realDistance = shadowPos.z; // bias was already applied when shadow map was rendered
	float3 shadow = 0;
#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a rectangle pattern around center:
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(-1, -1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(-1, 0) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(-1, 1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(0, -1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(0, 1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(1, -1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(1, 0) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV + float2(1, 1) * g_xFrame_ShadowKernel2D, slice), realDistance);
	shadow = shadow.xxx / 9.0;
#else
	shadow = texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(shadowUV, slice), realDistance);
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	if (g_xFrame_Options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED)
	{
		shadow *= texture_shadowarray_transparent.SampleLevel(sampler_linear_clamp, float3(shadowUV, slice), 0);
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

inline float shadowCube(in ShaderEntity light, in float3 L, in float3 Lunnormalized)
{
	const float slice = light.GetTextureIndex();
	float remappedDistance = light.GetCubemapDepthRemapNear() +
		light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid border sampling artifact
	float shadow = 0;
#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a cube pattern around center:
	L = -L;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(-1, -1, -1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(1, -1, -1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(-1, 1, -1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(1, 1, -1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L, slice), remappedDistance).r;
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(-1, -1, 1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(1, -1, 1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(-1, 1, 1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow += texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(L + float3(1, 1, 1) * g_xFrame_ShadowKernelCube, slice), remappedDistance);
	shadow /= 9.0;
#else
	shadow = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-Lunnormalized, slice), remappedDistance).r;
#endif // DISABLE_SOFT_SHADOWMAP
	return shadow;
}

inline float shadowTrace(in Surface surface, in float3 L, in float dist)
{
#ifdef RAYTRACING_INLINE
	RayQuery<
		RAY_FLAG_FORCE_OPAQUE | 
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
	> q;

	float seed = g_xFrame_FrameCount * 0.001;
	float2 uv = surface.screenUV;

	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = dist;
	ray.Origin = surface.P + surface.N * 0.01;

	float shadow = 0;
	uint sampleCount = 1;

#ifndef DISABLE_SOFT_RTSHADOW
	sampleCount = g_xFrame_RaytracedShadowsSampleCount;
	for (uint i = 0; i < sampleCount; ++i)
	{
		float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1; // todo: should be specific to light surface
		ray.Direction = normalize(L + sampling_offset * 0.025);
#else
	{
		ray.Direction = L;
#endif // DISABLE_SOFT_RTSHADOW

		q.TraceRayInline(scene_acceleration_structure, 0, 0xFF, ray);
		q.Proceed();

		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
		{
			shadow += 1;
		}
	}

	return shadow / sampleCount;
#endif // RAYTRACING_INLINE

	return 1;
}


inline void DirectionalLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.GetDirection().xyz;

	SurfaceToLight surfaceToLight;
	surfaceToLight.create(surface, L);

	[branch]
	if (any(surfaceToLight.NdotL_sss))
	{
		float3 shadow = 1;

		[branch]
		if (light.IsCastingShadow())
		{
			[branch]
			if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
			{
				shadow *= shadowTrace(surface, normalize(L), FLT_MAX);
			}
			else
			{
				// Loop through cascades from closest (smallest) to furthest (biggest)
				[loop]
				for (uint cascade = 0; cascade < g_xFrame_ShadowCascadeCount; ++cascade)
				{
					// Project into shadow map space (no need to divide by .w because ortho projection!):
					float3 ShPos = mul(MatrixArray[light.GetMatrixIndex() + cascade], float4(surface.P, 1)).xyz;
					float3 ShTex = ShPos * float3(0.5, -0.5, 0.5) + 0.5;

					// Determine if pixel is inside current cascade bounds and compute shadow if it is:
					[branch]
					if (is_saturated(ShTex))
					{
						const float3 shadow_main = shadowCascade(light, ShPos, ShTex.xy, cascade);
						const float3 cascade_edgefactor = saturate(saturate(abs(ShPos)) - 0.8) * 5.0; // fade will be on edge and inwards 20%
						const float cascade_fade = max(cascade_edgefactor.x, max(cascade_edgefactor.y, cascade_edgefactor.z));

						// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
						[branch]
						if (cascade_fade > 0 && cascade < g_xFrame_ShadowCascadeCount - 1)
						{
							// Project into next shadow cascade (no need to divide by .w because ortho projection!):
							cascade += 1;
							ShPos = mul(MatrixArray[light.GetMatrixIndex() + cascade], float4(surface.P, 1)).xyz;
							ShTex = ShPos * float3(0.5, -0.5, 0.5) + 0.5;
							const float3 shadow_fallback = shadowCascade(light, ShPos, ShTex.xy, cascade);

							shadow *= lerp(shadow_main, shadow_fallback, cascade_fade);
						}
						else
						{
							shadow *= shadow_main;
						}
						break;
					}
				}
			}
		}

		[branch]
		if (any(shadow))
		{
			float3 atmosphereTransmittance = 1;
			if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
			{
				AtmosphereParameters Atmosphere = GetAtmosphereParameters();
				atmosphereTransmittance = GetAtmosphericLightTransmittance(Atmosphere, surface.P, L, texture_transmittancelut);
			}
			
			float3 lightColor = light.GetColor().rgb * light.GetEnergy() * shadow * atmosphereTransmittance;

			lighting.direct.diffuse +=
				max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

			lighting.direct.specular +=
				max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
		}
	}
}
inline void PointLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float3 Lunnormalized = L;
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surfaceToLight;
		surfaceToLight.create(surface, L);

		[branch]
		if (any(surfaceToLight.NdotL_sss))
		{
			float shadow = 1;

			[branch]
			if (light.IsCastingShadow())
			{
				[branch]
				if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
				{
					shadow *= shadowTrace(surface, L, dist);
				}
				else
				{
					shadow *= shadowCube(light, L, Lunnormalized);
				}
			}

			[branch]
			if (any(shadow))
			{
				float3 lightColor = light.GetColor().rgb * light.GetEnergy() * shadow;

				const float att = saturate(1 - (dist2 / range2));
				const float attenuation = att * att;
				lightColor *= attenuation;

				lighting.direct.diffuse +=
					max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

				lighting.direct.specular +=
					max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
			}
		}
	}
}
inline void SpotLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.position - surface.P;
	const float dist2 = dot(L, L);
	const float range2 = light.GetRange() * light.GetRange();

	[branch]
	if (dist2 < range2)
	{
		const float dist = sqrt(dist2);
		L /= dist;

		SurfaceToLight surfaceToLight;
		surfaceToLight.create(surface, L);

		[branch]
		if (any(surfaceToLight.NdotL_sss))
		{
			const float SpotFactor = dot(L, light.GetDirection());
			const float spotCutOff = light.GetConeAngleCos();

			[branch]
			if (SpotFactor > spotCutOff)
			{
				float3 shadow = 1;

				[branch]
				if (light.IsCastingShadow())
				{
					[branch]
					if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
					{
						shadow *= shadowTrace(surface, L, dist);
					}
					else
					{
						float4 ShPos = mul(MatrixArray[light.GetMatrixIndex() + 0], float4(surface.P, 1));
						ShPos.xyz /= ShPos.w;
						float2 ShTex = ShPos.xy * float2(0.5, -0.5) + 0.5;
						[branch]
						if (is_saturated(ShTex))
						{
							shadow *= shadowCascade(light, ShPos.xyz, ShTex.xy, 0);
						}
					}
				}

				[branch]
				if (any(shadow))
				{
					float3 lightColor = light.GetColor().rgb * light.GetEnergy() * shadow;

					const float att = saturate(1 - (dist2 / range2));
					float attenuation = att * att;
					attenuation *= saturate((1 - (1 - SpotFactor) * 1 / (1 - spotCutOff)));
					lightColor *= attenuation;

					lighting.direct.diffuse +=
						max(0, lightColor * surfaceToLight.NdotL_sss * BRDF_GetDiffuse(surface, surfaceToLight));

					lighting.direct.specular +=
						max(0, lightColor * surfaceToLight.NdotL * BRDF_GetSpecular(surface, surfaceToLight));
				}
			}
		}
	}
}



#if 0 // Currently area lights are disabled
// AREA LIGHTS

// Based on the Frostbite presentation: 
//		Moving Frostbite to Physically Based Rendering by Sebastien Lagarde, Charles de Rousiers, Siggraph 2014
//		http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
//
// Representative point technique for speculars based on Brian Karis's Siggraph presentation:
//		http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//
// This is not completely physically correct rendering, but nice enough for now.

float cot(float x) { return cos(x) / sin(x); }
float acot(float x) { return atan(1 / x); }

float RightPyramidSolidAngle(float dist, float halfWidth, float halfHeight)
{
	float a = halfWidth;
	float b = halfHeight;
	float h = dist;

	return 4 * asin(a * b / sqrt((a * a + h * h) * (b * b + h * h)));
}
float RectangleSolidAngle(
	float3 worldPos,
	float3 p0, float3 p1,
	float3 p2, float3 p3)
{
	float3 v0 = p0 - worldPos;
	float3 v1 = p1 - worldPos;
	float3 v2 = p2 - worldPos;
	float3 v3 = p3 - worldPos;

	float3 n0 = normalize(cross(v0, v1));
	float3 n1 = normalize(cross(v1, v2));
	float3 n2 = normalize(cross(v2, v3));
	float3 n3 = normalize(cross(v3, v0));


	float g0 = acos(dot(-n0, n1));
	float g1 = acos(dot(-n1, n2));
	float g2 = acos(dot(-n2, n3));
	float g3 = acos(dot(-n3, n0));

	return g0 + g1 + g2 + g3 - 2 * PI;
}


inline float3 getDiffuseDominantDir(float3 N, float3 V, float NdotV, float roughness)
{
	float a = 1.02341f * roughness - 1.51174f;
	float b = -0.511705f * roughness + 0.755868f;
	float lerpFactor = saturate((NdotV * a + b) * roughness);

	return normalize(lerp(N, V, lerpFactor));
}
inline float3 getSpecularDominantDirArea(float3 N, float3 R, float roughness)
{
	// Simple linear approximation 
	float lerpFactor = (1 - roughness);

	return normalize(lerp(N, R, lerpFactor));
}

float illuminanceSphereOrDisk(float cosTheta, float sinSigmaSqr)
{
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	float illuminance = 0.0f;
	// Note: Following test is equivalent to the original formula. 
	// There is 3 phase in the curve: cosTheta > sqrt(sinSigmaSqr), 
	// cosTheta > -sqrt(sinSigmaSqr) and else it is 0 
	// The two outer case can be merge into a cosTheta * cosTheta > sinSigmaSqr 
	// and using saturate(cosTheta) instead. 
	if (cosTheta * cosTheta > sinSigmaSqr)
	{
		illuminance = PI * sinSigmaSqr * saturate(cosTheta);
	}
	else
	{
		float x = sqrt(1.0f / sinSigmaSqr - 1.0f); // For a disk this simplify to x = d / r 
		float y = -x * (cosTheta / sinTheta);
		float sinThetaSqrtY = sinTheta * sqrt(1.0f - y * y);
		illuminance = (cosTheta * acos(y) - x * sinThetaSqrtY) * sinSigmaSqr + atan(sinThetaSqrtY / x);
	}

	return max(illuminance, 0.0f);
}

inline void SphereLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_AREALIGHT
	float3 Lunnormalized = light.positionWS - surface.P;
	float dist = length(Lunnormalized);
	float3 L = Lunnormalized / dist;
	float fLight = 1;

	[branch]
	if (light.IsCastingShadow()) 
	{
		[branch]
		if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
		{
			fLight = shadowTrace(surface, L, dist);
		}
		else
		{
			fLight = shadowCube(light, L, Lunnormalized);
		}
	}

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunnormalized, Lunnormalized);

		float cosTheta = clamp(dot(surface.N, L), -0.999, 0.999); // Clamp to avoid edge case 
																// We need to prevent the object penetrating into the surface 
																// and we must avoid divide by 0, thus the 0.9999f 
		float sqrLightRadius = light.GetRadius() * light.GetRadius();
		float sinSigmaSqr = min(sqrLightRadius / sqrDist, 0.9999);
		fLight *= illuminanceSphereOrDisk(cosTheta, sinSigmaSqr);

		[branch]
		if (fLight > 0)
		{
			// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
			{
				float3 r = surface.R;
				r = getSpecularDominantDirArea(surface.N, r, surface.roughness);

				float3 centerToRay = dot(Lunnormalized, r) * r - Lunnormalized;
				float3 closestPoint = Lunnormalized + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.GetEnergy() * fLight;


			SurfaceToLight surfaceToLight;
			surfaceToLight.Create(surface, L);

			lighting.direct.specular += max(0, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
#endif // DISABLE_AREALIGHT
}
inline void DiscLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_AREALIGHT
	float3 Lunnormalized = light.positionWS - surface.P;
	float dist = length(Lunnormalized);
	float3 L = Lunnormalized / dist;
	float fLight = 1;

	[branch]
	if (light.IsCastingShadow())
	{
		[branch]
		if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
		{
			fLight = shadowTrace(surface, L, dist);
		}
		else
		{
			fLight = shadowCube(light, L, Lunnormalized);
		}
	}

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunnormalized, Lunnormalized);

		float3 lightPlaneNormal = light.GetFront();

		float cosTheta = clamp(dot(surface.N, L), -0.999, 0.999);
		float sqrLightRadius = light.GetRadius() * light.GetRadius();
		// Do not let the surface penetrate the light 
		float sinSigmaSqr = sqrLightRadius / (sqrLightRadius + max(sqrLightRadius, sqrDist));
		// Multiply by saturate(dot(planeNormal , -L)) to better match ground truth. 
		fLight *= illuminanceSphereOrDisk(cosTheta, sinSigmaSqr) * saturate(dot(lightPlaneNormal, -L));

		[branch]
		if (fLight > 0)
		{
			float3 r = surface.R;
			r = getSpecularDominantDirArea(surface.N, r, surface.roughness);
			float specularAttenuation = saturate(abs(dot(lightPlaneNormal, r))); // if ray is perpendicular to light plane, it would break specular, so fade in that case

			// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
			[branch]
			if(specularAttenuation > 0)
			{
				float t = Trace_plane(surface.P, r, light.positionWS, lightPlaneNormal);
				float3 p = surface.P + r * t;
				float3 centerToRay = p - light.positionWS;
				float3 closestPoint = Lunnormalized + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.GetEnergy() * fLight;

			SurfaceToLight surfaceToLight;
			surfaceToLight.Create(surface, L);

			lighting.direct.specular += max(0, specularAttenuation * lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
#endif // DISABLE_AREALIGHT
}
inline void RectangleLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_AREALIGHT
	float3 L = light.positionWS - surface.P;
	const float3 Lunnormalized = L;
	float dist = length(L);
	L /= dist;
	float fLight = 1;

	[branch]
	if (light.IsCastingShadow())
	{
		[branch]
		if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
		{
			fLight = shadowTrace(surface, L, dist);
		}
		else
		{
			fLight = shadowCube(light, L, Lunnormalized);
		}
	}

	[branch]
	if (fLight > 0)
	{
		float3 lightPlaneNormal = light.GetFront();
		float3 lightLeft = light.GetRight();
		float3 lightUp = light.GetUp();
		float lightWidth = light.GetWidth();
		float lightHeight = light.GetHeight();
		float3 worldPos = surface.P;
		float3 worldNormal = surface.N;


		float halfWidth = lightWidth * 0.5;
		float halfHeight = lightHeight * 0.5;
		float3 p0 = light.positionWS + lightLeft * -halfWidth + lightUp * halfHeight;
		float3 p1 = light.positionWS + lightLeft * -halfWidth + lightUp * -halfHeight;

		float3 p2 = light.positionWS + lightLeft * halfWidth + lightUp * -halfHeight;
		float3 p3 = light.positionWS + lightLeft * halfWidth + lightUp * halfHeight;
		float solidAngle = RectangleSolidAngle(worldPos, p0, p1, p2, p3);

		if (dot(worldPos - light.positionWS, lightPlaneNormal) > 0)
		{
			fLight *= solidAngle * 0.2 * (
				saturate(dot(normalize(p0 - worldPos), worldNormal)) +
				saturate(dot(normalize(p1 - worldPos), worldNormal)) +
				saturate(dot(normalize(p2 - worldPos), worldNormal)) +
				saturate(dot(normalize(p3 - worldPos), worldNormal)) +
				saturate(dot(normalize(light.positionWS - worldPos), worldNormal)));
		}
		else
		{
			fLight = 0;
		}

		[branch]
		if (fLight > 0)
		{
			float3 r = surface.R;
			r = getSpecularDominantDirArea(surface.N, r, surface.roughness);
			float specularAttenuation = saturate(abs(dot(lightPlaneNormal, r))); // if ray is perpendicular to light plane, it would break specular, so fade in that case

			// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
			[branch]
			if(specularAttenuation > 0)
			{
				float traced = Trace_rectangle(surface.P, r, p0, p1, p2, p3);

				[branch]
				if (traced > 0)
				{
					// Trace succeeded so the light vector L is the reflection vector itself
					L = r;
				}
				else
				{
					// The trace didn't succeed, so we need to find the closest point to the ray on the rectangle

					// We find the intersection point on the plane of the rectangle
					float3 tracedPlane = surface.P + r * Trace_plane(surface.P, r, light.positionWS, lightPlaneNormal);

					// Then find the closest point along the edges of the rectangle (edge = segment)
					float3 PC[4] = {
						ClosestPointOnSegment(p0, p1, tracedPlane),
						ClosestPointOnSegment(p1, p2, tracedPlane),
						ClosestPointOnSegment(p2, p3, tracedPlane),
						ClosestPointOnSegment(p3, p0, tracedPlane),
					};
					float dist[4] = {
						distance(PC[0], tracedPlane),
						distance(PC[1], tracedPlane),
						distance(PC[2], tracedPlane),
						distance(PC[3], tracedPlane),
					};

					float3 min = PC[0];
					float minDist = dist[0];
					[unroll]
					for (uint iLoop = 1; iLoop < 4; iLoop++)
					{
						if (dist[iLoop] < minDist)
						{
							minDist = dist[iLoop];
							min = PC[iLoop];
						}
					}

					L = min - surface.P;
				}
				L = normalize(L); // TODO: Is it necessary?
			}

			float3 lightColor = light.GetColor().rgb * light.GetEnergy() * fLight;

			SurfaceToLight surfaceToLight;
			surfaceToLight.Create(surface, L);

			lighting.direct.specular += max(0, specularAttenuation * lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
#endif // DISABLE_AREALIGHT
}
inline void TubeLight(in ShaderEntity light, in Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_AREALIGHT
	float3 Lunnormalized = light.positionWS - surface.P;
	float dist = length(Lunnormalized);
	float3 L = Lunnormalized / dist;
	float fLight = 1;

	[branch]
	if (light.IsCastingShadow())
	{
		[branch]
		if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
		{
			fLight = shadowTrace(surface, L, dist);
		}
		else
		{
			fLight = shadowCube(light, L, Lunnormalized);
		}
	}

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunnormalized, Lunnormalized);

		float3 lightLeft = light.GetRight();
		float lightWidth = light.GetWidth();
		float3 worldPos = surface.P;
		float3 worldNormal = surface.N;


		float3 P0 = light.positionWS - lightLeft * lightWidth * 0.5;
		float3 P1 = light.positionWS + lightLeft * lightWidth * 0.5;

		// The sphere is placed at the nearest point on the segment. 
		// The rectangular plane is define by the following orthonormal frame: 
		float3 forward = normalize(ClosestPointOnLine(P0, P1, worldPos) - worldPos);
		float3 left = lightLeft;
		float3 up = cross(lightLeft, forward);

		float3 p0 = light.positionWS - left * (0.5 * lightWidth) + light.GetRadius() * up;
		float3 p1 = light.positionWS - left * (0.5 * lightWidth) - light.GetRadius() * up;
		float3 p2 = light.positionWS + left * (0.5 * lightWidth) - light.GetRadius() * up;
		float3 p3 = light.positionWS + left * (0.5 * lightWidth) + light.GetRadius() * up;


		float solidAngle = RectangleSolidAngle(worldPos, p0, p1, p2, p3);

		fLight *= solidAngle * 0.2 * (
			saturate(dot(normalize(p0 - worldPos), worldNormal)) +
			saturate(dot(normalize(p1 - worldPos), worldNormal)) +
			saturate(dot(normalize(p2 - worldPos), worldNormal)) +
			saturate(dot(normalize(p3 - worldPos), worldNormal)) +
			saturate(dot(normalize(light.positionWS - worldPos), worldNormal)));

		// We then add the contribution of the sphere 
		float3 spherePosition = ClosestPointOnSegment(P0, P1, worldPos);
		float3 sphereUnormL = spherePosition - worldPos;
		float3 sphereL = normalize(sphereUnormL);
		float sqrSphereDistance = dot(sphereUnormL, sphereUnormL);

		float fLightSphere = PI * saturate(dot(sphereL, worldNormal)) * ((light.GetRadius() * light.GetRadius()) / sqrSphereDistance);
		fLight += fLightSphere;

		[branch]
		if (fLight > 0)
		{
			// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
			{
				float3 r = surface.R;
				r = getSpecularDominantDirArea(surface.N, r, surface.roughness);

				// First, the closest point to the ray on the segment
				float3 L0 = P0 - surface.P;
				float3 L1 = P1 - surface.P;
				float3 Ld = L1 - L0;

				float t = dot(r, L0) * dot(r, Ld) - dot(L0, Ld);
				t /= dot(Ld, Ld) - sqr(dot(r, Ld));

				L = (L0 + saturate(t) * Ld);

				// Then I place a sphere on that point and calculate the light vector like for sphere light.
				float3 centerToRay = dot(L, r) * r - L;
				float3 closestPoint = L + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.GetEnergy() * fLight;

			SurfaceToLight surfaceToLight;
			surfaceToLight.Create(surface, L);

			lighting.direct.specular += max(0, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
#endif // DISABLE_AREALIGHT
}
#endif // AREA LIGHTS


// VOXEL RADIANCE

inline void VoxelGI(in Surface surface, inout Lighting lighting)
{
	[branch]if (g_xFrame_VoxelRadianceDataRes != 0)
	{
		// determine blending factor (we will blend out voxel GI on grid edges):
		float3 voxelSpacePos = surface.P - g_xFrame_VoxelRadianceDataCenter;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataSize_rcp;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataRes_rcp;
		voxelSpacePos = saturate(abs(voxelSpacePos));
		float blend = 1 - pow(max(voxelSpacePos.x, max(voxelSpacePos.y, voxelSpacePos.z)), 4);

		float4 radiance = ConeTraceRadiance(texture_voxelradiance, surface.P, surface.N);

		lighting.indirect.diffuse = lerp(lighting.indirect.diffuse, radiance.rgb, radiance.a * blend);


		[branch]
		if (g_xFrame_Options & OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED)
		{
			float4 reflection = ConeTraceReflection(texture_voxelradiance, surface.P, surface.N, surface.V, surface.roughness);

			lighting.indirect.specular = lerp(lighting.indirect.specular, reflection.rgb, reflection.a * blend);
		}
	}
}


// ENVIRONMENT MAPS


inline float3 GetAmbient(in float3 N)
{
	float3 ambient;

#ifndef ENVMAPRENDERING
	[branch]
	if (g_xFrame_GlobalEnvProbeIndex >= 0)
	{
		ambient = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(N, g_xFrame_GlobalEnvProbeIndex), g_xFrame_EnvProbeMipCount).rgb;
	}
	else
#endif // ENVMAPRENDERING
	{
		// Also set realistic_sky_stationary to true so we capture ambient at float3(0.0, 0.0, 0.0), similar to the standard sky to avoid flickering and weird behavior
		ambient = lerp(
			GetDynamicSkyColor(float3(0, -1, 0), false, false, false, true),
			GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
			saturate(N.y * 0.5 + 0.5)) + GetAmbientColor();
	}

	return ambient;
}

// surface:				surface descriptor
// MIP:					mip level to sample
// return:				color of the environment color (rgb)
inline float3 EnvironmentReflection_Global(in Surface surface, in float MIP)
{
	float3 envColor;

#ifndef ENVMAPRENDERING
	[branch]
	if (g_xFrame_GlobalEnvProbeIndex >= 0)
	{
		// We have envmap information in a texture:
		envColor = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(surface.R, g_xFrame_GlobalEnvProbeIndex), MIP).rgb;
	}
	else
#endif // ENVMAPRENDERING
	{
		// There are no envmaps, approximate sky color:
		// Also set realistic_sky_stationary to true so we capture environment at float3(0.0, 0.0, 0.0), similar to the standard sky to avoid flickering and weird behavior
		float3 realSkyColor = GetDynamicSkyColor(surface.R, false, false, false, true); // false: disable sun disk and clouds
		float3 roughSkyColor = lerp(
			GetDynamicSkyColor(float3(0, -1, 0), false, false, false, true),
			GetDynamicSkyColor(float3(0, 1, 0), false, false, false, true),
			saturate(surface.R.y * 0.5 + 0.5));
		
		envColor = lerp(realSkyColor, roughSkyColor, saturate(surface.roughness));
	}

	return envColor;
}

// surface:				surface descriptor
// probe :				the shader entity holding properties
// probeProjection:		the inverse OBB transform matrix
// clipSpacePos:		world space pixel position transformed into OBB space by probeProjection matrix
// MIP:					mip level to sample
// return:				color of the environment map (rgb), blend factor of the environment map (a)
inline float4 EnvironmentReflection_Local(in Surface surface, in ShaderEntity probe, in float4x4 probeProjection, in float3 clipSpacePos, in float MIP)
{
	// Perform parallax correction of reflection ray (R) into OBB:
	float3 RayLS = mul(surface.R, (float3x3)probeProjection);
	float3 FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	float3 IntersectPositionWS = surface.P + surface.R * Distance;
	float3 R_parallaxCorrected = IntersectPositionWS - probe.position;

	// Sample cubemap texture:
	float3 envmapColor = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(R_parallaxCorrected, probe.GetTextureIndex()), MIP).rgb; // GetFlags() stores textureIndex here...
	// blend out if close to any cube edge:
	float edgeBlend = 1 - pow(saturate(max(abs(clipSpacePos.x), max(abs(clipSpacePos.y), abs(clipSpacePos.z)))), 8);

	return float4(envmapColor, edgeBlend);
}

#endif // WI_LIGHTING_HF
