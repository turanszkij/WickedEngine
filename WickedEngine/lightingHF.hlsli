#ifndef _LIGHTING_HF_
#define _LIGHTING_HF_
#include "globals.hlsli"
#include "brdf.hlsli"
#include "voxelConeTracingHF.hlsli"

struct LightingContribution
{
	float diffuse;
	float specular;
};
struct LightingPart
{
	float3 diffuse;
	float3 specular;
};
struct Lighting
{
	LightingPart direct;
	LightingPart indirect;
};

inline Lighting CreateLighting(
	in float3 diffuse_direct,
	in float3 specular_direct,
	in float3 diffuse_indirect,
	in float3 specular_indirect)
{
	Lighting lighting;
	lighting.direct.diffuse = diffuse_direct;
	lighting.direct.specular = specular_direct;
	lighting.indirect.diffuse = diffuse_indirect;
	lighting.indirect.specular = specular_indirect;
	return lighting;
}

// Combine the direct and indirect lighting into final contribution
inline LightingPart CombineLighting(in Surface surface, in Lighting lighting)
{
	LightingPart result;
	result.diffuse = lighting.direct.diffuse + lighting.indirect.diffuse * surface.occlusion;
	result.specular = lighting.direct.specular + lighting.indirect.specular * surface.F * surface.occlusion;
	return result;
}



inline float3 shadowCascade(float3 shadowPos, float2 ShTex, float shadowKernel, float bias, float slice) 
{
	const float realDistance = shadowPos.z + bias;
	float3 shadow = 0;
#ifndef DISABLE_SHADOWMAPS
#ifndef DISABLE_SOFT_SHADOWS
	const float range = 1.5f;
	[loop]
	for (float y = -range; y <= range; y += 1.0f)
	{
		[loop]
		for (float x = -range; x <= range; x += 1.0f)
		{
			shadow.x += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex + float2(x, y) * shadowKernel, slice), realDistance).r;
			shadow.y++;
		}
	}
	shadow = shadow.x / shadow.y;
#else
	shadow = texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex, slice), realDistance).r;
#endif

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	if (g_xFrame_TransparentShadowsEnabled)
	{
		// unfortunately transparents will not receive transparent shadow map
		// because we cannot distinguish without using secondary depth buffer for transparents
		// but I don't wanna do that, not overly important for now
		float4 transparent_shadowmap = texture_shadowarray_transparent.SampleLevel(sampler_linear_clamp, float3(ShTex, slice), 0).rgba;
		// Tint the shadow:
		shadow *= transparent_shadowmap.rgb;
		// Reduce shadow by caustics (caustics can also increase total light above maximum):
		const float causticsStrength = 20;
		shadow += transparent_shadowmap.a * causticsStrength;
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

#endif // DISABLE_SHADOWMAPS

	return shadow;
}


inline void DirectionalLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.directionWS.xyz;
	SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

	[branch]
	if (surfaceToLight.NdotL > 0)
	{
		float3 sh = surfaceToLight.NdotL.xxx;

#ifndef DISABLE_SHADOWMAPS
		[branch]
		if (light.IsCastingShadow())
		{
			// Loop through cascades from closest (smallest) to farest (biggest)
			[loop]
			for (uint cascade = 0; cascade < g_xFrame_ShadowCascadeCount; ++cascade)
			{
				// Project into shadow map space (no need to divide by .w because ortho projection!):
				float3 ShPos = mul(float4(surface.P, 1), MatrixArray[light.GetShadowMatrixIndex() + cascade]).xyz;
				float3 ShTex = ShPos * float3(0.5f, -0.5f, 0.5f) + 0.5f;

				// Determine if pixel is inside current cascade bounds and compute shadow if it is:
				[branch]
				if (is_saturated(ShTex))
				{
					const float3 shadow_main = shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.GetShadowMapIndex() + cascade);
					const float3 cascade_edgefactor = saturate(saturate(abs(ShPos)) - 0.8f) * 5.0f; // fade will be on edge and inwards 20%
					const float cascade_fade = max(cascade_edgefactor.x, max(cascade_edgefactor.y, cascade_edgefactor.z));

					// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
					[branch]
					if (cascade_fade > 0 && cascade < g_xFrame_ShadowCascadeCount - 1)
					{
						// Project into next shadow cascade (no need to divide by .w because ortho projection!):
						cascade += 1;
						ShPos = mul(float4(surface.P, 1), MatrixArray[light.GetShadowMatrixIndex() + cascade]).xyz;
						ShTex = ShPos * float3(0.5f, -0.5f, 0.5f) + 0.5f;
						const float3 shadow_fallback = shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.GetShadowMapIndex() + cascade);

						sh *= lerp(shadow_main, shadow_fallback, cascade_fade);
					}
					else
					{
						sh *= shadow_main;
					}
					break;
				}
			}

		}
#endif // DISABLE_SHADOWMAPS

		[branch]
		if (any(sh))
		{
			float3 lightColor = light.GetColor().rgb * light.energy * sh;
			lighting.direct.diffuse += max(0.0f, lightColor * BRDF_GetDiffuse(surface, surfaceToLight));
			lighting.direct.specular += max(0.0f, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
		}
	}
}
inline void PointLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.positionWS - surface.P;
	const float dist2 = dot(L, L);
	const float dist = sqrt(dist2);

	[branch]
	if (dist < light.range)
	{
		L /= dist;

		SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

		[branch]
		if (surfaceToLight.NdotL > 0)
		{
			float sh = surfaceToLight.NdotL;

#ifndef DISABLE_SHADOWMAPS
			[branch]
			if (light.IsCastingShadow()) {
				sh *= texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.GetShadowMapIndex()), 1 - dist / light.range * (1 - light.shadowBias)).r;
			}
#endif // DISABLE_SHADOWMAPS

			[branch]
			if (sh > 0)
			{
				float3 lightColor = light.GetColor().rgb * light.energy * sh;

				const float range2 = light.range * light.range;
				const float att = saturate(1.0 - (dist2 / range2));
				const float attenuation = att * att;
				lightColor *= attenuation;

				lighting.direct.diffuse += max(0.0f, lightColor * BRDF_GetDiffuse(surface, surfaceToLight));
				lighting.direct.specular += max(0.0f, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			}
		}
	}
}
inline void SpotLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.positionWS - surface.P;
	const float dist2 = dot(L, L);
	const float dist = sqrt(dist2);

	[branch]
	if (dist < light.range)
	{
		L /= dist;

		SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

		[branch]
		if (surfaceToLight.NdotL > 0)
		{
			const float SpotFactor = dot(L, light.directionWS);
			const float spotCutOff = light.coneAngleCos;

			[branch]
			if (SpotFactor > spotCutOff)
			{
				float3 sh = surfaceToLight.NdotL.xxx;

#ifndef DISABLE_SHADOWMAPS
				[branch]
				if (light.IsCastingShadow())
				{
					float4 ShPos = mul(float4(surface.P, 1), MatrixArray[light.GetShadowMatrixIndex() + 0]);
					ShPos.xyz /= ShPos.w;
					float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
					[branch]
					if (is_saturated(ShTex))
					{
						sh *= shadowCascade(ShPos.xyz, ShTex.xy, light.shadowKernel, light.shadowBias, light.GetShadowMapIndex());
					}
				}
#endif // DISABLE_SHADOWMAPS

				[branch]
				if (any(sh))
				{
					float3 lightColor = light.GetColor().rgb * light.energy * sh;

					const float range2 = light.range * light.range;
					const float att = saturate(1.0 - (dist2 / range2));
					float attenuation = att * att;
					attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));
					lightColor *= attenuation;

					lighting.direct.diffuse += max(0.0f, lightColor * BRDF_GetDiffuse(surface, surfaceToLight));
					lighting.direct.specular += max(0.0f, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
				}
			}
		}
	}
}




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

// Return the closest point on the line (without limit) 
float3 ClosestPointOnLine(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + t * ab;
}
// Return the closest point on the segment (with limit) 
float3 ClosestPointOnSegment(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + saturate(t) * ab;
}
float RightPyramidSolidAngle(float dist, float halfWidth, float halfHeight)
{
	float a = halfWidth;
	float b = halfHeight;
	float h = dist;

	return 4 * asin(a * b / sqrt((a * a + h * h) * (b * b + h * h)));
}
float RectangleSolidAngle(float3 worldPos,
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


// o		: ray origin
// d		: ray direction
// center	: sphere center
// radius	: sphere radius
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_sphere(float3 o, float3 d, float3 center, float radius)
{
	float3 rc = o - center;
	float c = dot(rc, rc) - (radius*radius);
	float b = dot(d, rc);
	float dd = b*b - c;
	float t = -b - sqrt(abs(dd));
	float st = step(0.0, min(t, dd));
	return lerp(-1.0, t, st);
}
// o		: ray origin
// d		: ray direction
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_plane(float3 o, float3 d, float3 planeOrigin, float3 planeNormal)
{
	return dot(planeNormal, (planeOrigin - o) / dot(planeNormal, d));
}
// o		: ray origin
// d		: ray direction
// A,B,C	: traingle corners
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_triangle(float3 o, float3 d, float3 A, float3 B, float3 C)
{
	float3 planeNormal = normalize(cross(B - A, C - B));
	float t = Trace_plane(o, d, A, planeNormal);
	float3 p = o + d*t;

	float3 N1 = normalize(cross(B - A, p - B));
	float3 N2 = normalize(cross(C - B, p - C));
	float3 N3 = normalize(cross(A - C, p - A));

	float d0 = dot(N1, N2);
	float d1 = dot(N2, N3);

	float threshold = 1.0f - 0.001f;
	return (d0 > threshold && d1 > threshold) ? 1.0f : 0.0f;
}
// o		: ray origin
// d		: ray direction
// A,B,C,D	: rectangle corners
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_rectangle(float3 o, float3 d, float3 A, float3 B, float3 C, float3 D)
{
	return max(Trace_triangle(o, d, A, B, C), Trace_triangle(o, d, C, D, A));
}
// o		: ray origin
// d		: ray direction
// diskNormal : disk facing direction
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_disk(float3 o, float3 d, float3 diskCenter, float diskRadius, float3 diskNormal)
{
	float t = Trace_plane(o, d, diskCenter, diskNormal);
	float3 p = o + d*t;
	float3 diff = p - diskCenter;
	return dot(diff, diff)<sqr(diskRadius);
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

inline void SphereLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 Lunormalized = light.positionWS - surface.P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;
	float fLight = 1;

#ifndef DISABLE_SHADOWMAPS
	[branch]
	if (light.IsCastingShadow()) {
		fLight = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.GetShadowMapIndex()), 1 - dist / light.range * (1 - light.shadowBias)).r;
	}
#endif

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunormalized, Lunormalized);

		float cosTheta = clamp(dot(surface.N, L), -0.999, 0.999); // Clamp to avoid edge case 
																// We need to prevent the object penetrating into the surface 
																// and we must avoid divide by 0, thus the 0.9999f 
		float sqrLightRadius = light.GetRadius() * light.GetRadius();
		float sinSigmaSqr = min(sqrLightRadius / sqrDist, 0.9999f);
		fLight *= illuminanceSphereOrDisk(cosTheta, sinSigmaSqr);

		[branch]
		if (fLight > 0)
		{
			// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
			{
				float3 r = surface.R;
				r = getSpecularDominantDirArea(surface.N, r, surface.roughness);

				float3 centerToRay = dot(Lunormalized, r) * r - Lunormalized;
				float3 closestPoint = Lunormalized + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.energy * fLight;


			SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

			lighting.direct.specular += max(0, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
}
inline void DiscLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 Lunormalized = light.positionWS - surface.P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;
	float fLight = 1;

#ifndef DISABLE_SHADOWMAPS
	[branch]
	if (light.IsCastingShadow()) {
		fLight = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.GetShadowMapIndex()), 1 - dist / light.range * (1 - light.shadowBias)).r;
	}
#endif

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunormalized, Lunormalized);

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
				float3 closestPoint = Lunormalized + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.energy * fLight;

			SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

			lighting.direct.specular += max(0, specularAttenuation * lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
}
inline void RectangleLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 L = light.positionWS - surface.P;
	float dist = length(L);
	L /= dist;
	float fLight = 1;

#ifndef DISABLE_SHADOWMAPS
	[branch]
	if (light.IsCastingShadow()) {
		fLight = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.GetShadowMapIndex()), 1 - dist / light.range * (1 - light.shadowBias)).r;
	}
#endif

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

			float3 lightColor = light.GetColor().rgb * light.energy * fLight;

			SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

			lighting.direct.specular += max(0, specularAttenuation * lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
}
inline void TubeLight(in ShaderEntityType light, in Surface surface, inout Lighting lighting)
{
	float3 Lunormalized = light.positionWS - surface.P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;
	float fLight = 1;

#ifndef DISABLE_SHADOWMAPS
	[branch]
	if (light.IsCastingShadow()) {
		fLight = texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.GetShadowMapIndex()), 1 - dist / light.range * (1 - light.shadowBias)).r;
	}
#endif

	[branch]
	if (fLight > 0)
	{
		float sqrDist = dot(Lunormalized, Lunormalized);

		float3 lightLeft = light.GetRight();
		float lightWidth = light.GetWidth();
		float3 worldPos = surface.P;
		float3 worldNormal = surface.N;


		float3 P0 = light.positionWS - lightLeft * lightWidth*0.5f;
		float3 P1 = light.positionWS + lightLeft * lightWidth*0.5f;

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

				// Then I place a sphere on that point and calculate the lisght vector like for sphere light.
				float3 centerToRay = dot(L, r) * r - L;
				float3 closestPoint = L + centerToRay * saturate(light.GetRadius() / length(centerToRay));
				L = normalize(closestPoint);
			}

			float3 lightColor = light.GetColor().rgb * light.energy * fLight;

			SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

			lighting.direct.specular += max(0, lightColor * BRDF_GetSpecular(surface, surfaceToLight));
			lighting.direct.diffuse += max(0, lightColor / PI);
		}
	}
}


// VOXEL RADIANCE

inline LightingContribution VoxelGI(in Surface surface, inout Lighting lighting)
{
	LightingContribution contribution;

	[branch]if (g_xFrame_VoxelRadianceDataRes != 0)
	{
		// determine blending factor (we will blend out voxel GI on grid edges):
		float3 voxelSpacePos = surface.P - g_xFrame_VoxelRadianceDataCenter;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataSize_Inverse;
		voxelSpacePos *= g_xFrame_VoxelRadianceDataRes_Inverse;
		voxelSpacePos = saturate(abs(voxelSpacePos));
		float blend = 1 - pow(max(voxelSpacePos.x, max(voxelSpacePos.y, voxelSpacePos.z)), 4);

		float4 radiance = ConeTraceRadiance(texture_voxelradiance, surface.P, surface.N);

		contribution.diffuse = radiance.a * blend;

		lighting.indirect.diffuse = lerp(lighting.indirect.diffuse, radiance.rgb, contribution.diffuse);


		[branch]
		if (g_xFrame_VoxelRadianceReflectionsEnabled)
		{
			float4 reflection = ConeTraceReflection(texture_voxelradiance, surface.P, surface.N, surface.V, surface.roughness);

			contribution.specular = reflection.a * blend;

			lighting.indirect.specular = lerp(lighting.indirect.specular, reflection.rgb, contribution.specular);
		}
		else
		{
			contribution.specular = 0;
		}
	}
	else
	{
		contribution.diffuse = 0;
		contribution.specular = 0;
	}

	return contribution;
}


// ENVIRONMENT MAPS


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
		float3 realSkyColor = lerp(GetHorizonColor(), GetZenithColor(), pow(saturate(surface.R.y), 0.25f));
		float3 roughSkyColor = (GetHorizonColor() + GetZenithColor()) * 0.5f;
		float blendSkyByRoughness = saturate(MIP * g_xFrame_EnvProbeMipCount_Inverse);
		envColor = lerp(realSkyColor, roughSkyColor, blendSkyByRoughness);
	}

	return envColor;
}

// surface:				surface descriptor
// probe :				the shader entity holding properties
// probeProjection:		the inverse OBB transform matrix
// clipSpacePos:		world space pixel position transformed into OBB space by probeProjection matrix
// MIP:					mip level to sample
// return:				color of the environment map (rgb), blend factor of the environment map (a)
inline float4 EnvironmentReflection_Local(in Surface surface, in ShaderEntityType probe, in float4x4 probeProjection, in float3 clipSpacePos, in float MIP)
{
	// Perform parallax correction of reflection ray (R) into OBB:
	float3 RayLS = mul(surface.R, (float3x3)probeProjection);
	float3 FirstPlaneIntersect = (float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 SecondPlaneIntersect = (-float3(1, 1, 1) - clipSpacePos) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));
	float3 IntersectPositionWS = surface.P + surface.R * Distance;
	float3 R_parallaxCorrected = IntersectPositionWS - probe.positionWS;

	// Sample cubemap texture:
	float3 envmapColor = texture_envmaparray.SampleLevel(sampler_linear_clamp, float4(R_parallaxCorrected, probe.shadowBias), MIP).rgb; // shadowBias stores textureIndex here...
	// blend out if close to any cube edge:
	float edgeBlend = 1 - pow(saturate(max(abs(clipSpacePos.x), max(abs(clipSpacePos.y), abs(clipSpacePos.z)))), 8);

	return float4(envmapColor, edgeBlend);
}

#endif // _LIGHTING_HF_