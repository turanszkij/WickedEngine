#ifndef _LIGHTING_HF_
#define _LIGHTING_HF_
#include "globals.hlsli"
#include "brdf.hlsli"

struct LightArrayType
{
	float3 positionVS; // View Space!
	float range;
	// --
	float4 color;
	// --
	float3 positionWS;
	float energy;
	// --
	float3 directionVS;
	float shadowKernel;
	// --
	float3 directionWS;
	uint type;
	// --
	float shadowBias;
	int shadowMap_index;
	float coneAngle;
	float coneAngleCos;
	// --
	float4 texMulAdd;
	// --
	float4x4 shadowMat[3];
};

TEXTURE2D(LightGrid, uint2, TEXSLOT_LIGHTGRID);
STRUCTUREDBUFFER(LightIndexList, uint, SBSLOT_LIGHTINDEXLIST);
STRUCTUREDBUFFER(LightArray, LightArrayType, SBSLOT_LIGHTARRAY);

inline float3 GetSunColor()
{
	return LightArray[g_xFrame_SunLightArrayIndex].color.rgb;
}
inline float3 GetSunDirection()
{
	return LightArray[g_xFrame_SunLightArrayIndex].directionWS;
}

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

inline float shadowCascade(float4 shadowPos, float2 ShTex, float shadowKernel, float bias, float slice) 
{
	float realDistance = shadowPos.z - bias;
	float sum = 0;
	float retVal = 1; 
#ifdef DIRECTIONALLIGHT_SOFT
	float samples = 0.0f;
	static const float range = 1.5f;
	for (float y = -range; y <= range; y += 1.0f)
	{
		for (float x = -range; x <= range; x += 1.0f)
		{
			sum += texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex + float2(x, y) * shadowKernel, slice), realDistance).r;
			samples++;
		}
	}
	retVal *= sum / samples;
#else
	retVal *= texture_shadowarray_2d.SampleCmpLevelZero(sampler_cmp_depth, float3(ShTex, slice), realDistance).r;
#endif
	return retVal;
}


inline LightingResult DirectionalLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result;
	float3 lightColor = light.color.rgb*light.energy;

	float3 L = light.directionWS.xyz;
	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
	result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

	float sh = max(NdotL, 0);

	[branch]
	if (light.shadowMap_index >= 0)
	{
		float4 ShPos[3];
		ShPos[0] = mul(float4(P, 1), light.shadowMat[0]);
		ShPos[1] = mul(float4(P, 1), light.shadowMat[1]);
		ShPos[2] = mul(float4(P, 1), light.shadowMat[2]);
		ShPos[0].xyz /= ShPos[0].w;
		ShPos[1].xyz /= ShPos[1].w;
		ShPos[2].xyz /= ShPos[2].w;
		float3 ShTex[3];
		ShTex[0] = ShPos[0].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		ShTex[1] = ShPos[1].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		ShTex[2] = ShPos[2].xyz*float3(1, -1, 1) / 2.0f + 0.5f;
		[branch]if ((saturate(ShTex[2].x) == ShTex[2].x) && (saturate(ShTex[2].y) == ShTex[2].y) && (saturate(ShTex[2].z) == ShTex[2].z))
		{
			const float shadows[] = {
				shadowCascade(ShPos[1],ShTex[1].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 1),
				shadowCascade(ShPos[2],ShTex[2].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 2)
			};
			const float2 lerpVal = abs(ShTex[2].xy * 2 - 1);
			sh *= lerp(shadows[1], shadows[0], pow(max(lerpVal.x, lerpVal.y), 4));
		}
		else[branch]if ((saturate(ShTex[1].x) == ShTex[1].x) && (saturate(ShTex[1].y) == ShTex[1].y) && (saturate(ShTex[1].z) == ShTex[1].z))
		{
			const float shadows[] = {
				shadowCascade(ShPos[0],ShTex[0].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 0),
				shadowCascade(ShPos[1],ShTex[1].xy, light.shadowKernel,light.shadowBias,light.shadowMap_index + 1),
			};
			const float2 lerpVal = abs(ShTex[1].xy * 2 - 1);
			sh *= lerp(shadows[1], shadows[0], pow(max(lerpVal.x, lerpVal.y), 4));
		}
		else[branch]if ((saturate(ShTex[0].x) == ShTex[0].x) && (saturate(ShTex[0].y) == ShTex[0].y) && (saturate(ShTex[0].z) == ShTex[0].z))
		{
			sh *= shadowCascade(ShPos[0], ShTex[0].xy, light.shadowKernel, light.shadowBias, light.shadowMap_index + 0);
		}
	}

	result.diffuse *= sh;
	result.specular *= sh;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);
	return result;
}
inline LightingResult PointLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;

	float3 L = light.positionWS - P;
	float dist = length(L);

	[branch]
	if (dist < light.range)
	{
		L /= dist;

		float3 lightColor = light.color.rgb*light.energy;

		BRDF_MAKE(N, L, V);
		result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
		result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

		float att = (light.energy * (light.range / (light.range + 1 + dist)));
		float attenuation = (att * (light.range - dist) / light.range);
		result.diffuse *= attenuation;
		result.specular *= attenuation;

		float sh = max(NdotL, 0);
		[branch]
		if (light.shadowMap_index >= 0) {
			static const float bias = 0.025;
			sh *= texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.shadowMap_index), dist / light.range - bias).r;
		}
		result.diffuse *= sh;
		result.specular *= sh;

		result.diffuse = max(0.0f, result.diffuse);
		result.specular = max(0.0f, result.specular);
	}

	return result;
}
inline LightingResult SpotLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;

	float3 L = light.positionWS - P;
	float dist = length(L);

	[branch]
	if (dist < light.range)
	{
		L /= dist;

		float3 lightColor = light.color.rgb*light.energy;

		float SpotFactor = dot(L, light.directionWS);
		float spotCutOff = light.coneAngleCos;

		[branch]
		if (SpotFactor > spotCutOff)
		{

			BRDF_MAKE(N, L, V);
			result.specular = lightColor * BRDF_SPECULAR(roughness, f0);
			result.diffuse = lightColor * BRDF_DIFFUSE(roughness);

			float att = (light.energy * (light.range / (light.range + 1 + dist)));
			float attenuation = (att * (light.range - dist) / light.range);
			attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));
			result.diffuse *= attenuation;
			result.specular *= attenuation;

			float sh = max(NdotL, 0);
			[branch]
			if (light.shadowMap_index >= 0)
			{
				float4 ShPos = mul(float4(P, 1), light.shadowMat[0]);
				ShPos.xyz /= ShPos.w;
				float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
				[branch]
				if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
				{
					sh *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.shadowMap_index);
				}
			}
			result.diffuse *= sh;
			result.specular *= sh;

			result.diffuse = max(result.diffuse, 0);
			result.specular = max(result.specular, 0);
		}

		result.diffuse = max(0.0f, result.diffuse);
		result.specular = max(0.0f, result.specular);
	}

	return result;
}




// AREA LIGHTS

// Based on the Frostbite presentation: 
//		Moving Frostbite to Physically Based Rendering by Sebastien Lagarde, Charles de Rousiers, Siggraph 2014
//		http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf

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

inline float3 _GetLeft(LightArrayType light) { return light.directionWS; }
inline float3 _GetUp(LightArrayType light) { return light.directionVS; }
inline float3 _GetFront(LightArrayType light) { return light.positionVS; }
inline float _GetRadius(LightArrayType light) { return light.texMulAdd.x; }
inline float _GetWidth(LightArrayType light) { return light.texMulAdd.y; }
inline float _GetHeight(LightArrayType light) { return light.texMulAdd.z; }

inline LightingResult SphereLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;

	float3 Lunormalized = light.positionWS - P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;

	float sqrDist = dot(Lunormalized, Lunormalized);

	float cosTheta = clamp(dot(N, L), -0.999, 0.999); // Clamp to avoid edge case 
															// We need to prevent the object penetrating into the surface 
															// and we must avoid divide by 0, thus the 0.9999f 
	float sqrLightRadius = _GetRadius(light) * _GetRadius(light);
	float sinSigmaSqr = min(sqrLightRadius / sqrDist, 0.9999f);
	float fLight = illuminanceSphereOrDisk(cosTheta, sinSigmaSqr);


	// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
	{
		float3 r = reflect(V, N);
		r = getSpecularDominantDirArea(N, r, roughness);

		float3 centerToRay = dot(Lunormalized, r) * r - Lunormalized;
		float3 closestPoint = Lunormalized + centerToRay * saturate(_GetRadius(light) / length(centerToRay));
		L = normalize(closestPoint);
	}

	float3 lightColor = light.color.rgb*light.energy;

	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0) * fLight;
	result.diffuse = lightColor * fLight / PI;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);

	return result;
}
inline LightingResult DiscLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;

	float3 Lunormalized = light.positionWS - P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;

	float sqrDist = dot(Lunormalized, Lunormalized);

	float3 lightPlaneNormal = _GetFront(light);

	float cosTheta = clamp(dot(N, L), -0.999, 0.999);
	float sqrLightRadius = _GetRadius(light) * _GetRadius(light);
	// Do not let the surface penetrate the light 
	float sinSigmaSqr = sqrLightRadius / (sqrLightRadius + max(sqrLightRadius, sqrDist));
	// Multiply by saturate(dot(planeNormal , -L)) to better match ground truth. 
	float fLight = illuminanceSphereOrDisk(cosTheta, sinSigmaSqr)
		* saturate(dot(lightPlaneNormal, -L));

	// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
	{
		float3 r = reflect(V, N);
		r = getSpecularDominantDirArea(N, r, roughness);

		float t = Trace_plane(P, r, light.positionWS, lightPlaneNormal);
		float3 p = P + r*t;
		float3 centerToRay = p - light.positionWS;
		float3 closestPoint = Lunormalized + centerToRay * saturate(_GetRadius(light) / length(centerToRay));
		L = normalize(closestPoint);
	}

	float3 lightColor = light.color.rgb*light.energy;

	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0) * fLight;
	result.diffuse = lightColor * fLight / PI;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);

	return result;
}
inline LightingResult RectangleLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;


	float3 L = light.positionWS - P;
	float dist = length(L);
	L /= dist;


	float3 lightPlaneNormal = _GetFront(light);
	float3 lightLeft = _GetLeft(light);
	float3 lightUp = _GetUp(light);
	float lightWidth = _GetWidth(light);
	float lightHeight = _GetHeight(light);
	float3 worldPos = P;
	float3 worldNormal = N;


	float fLight = 0;
	float halfWidth = lightWidth * 0.5;
	float halfHeight = lightHeight * 0.5;
	float3 p0 = light.positionWS + lightLeft * -halfWidth + lightUp * halfHeight;
	float3 p1 = light.positionWS + lightLeft * -halfWidth + lightUp * -halfHeight;

	float3 p2 = light.positionWS + lightLeft * halfWidth + lightUp * -halfHeight;
	float3 p3 = light.positionWS + lightLeft * halfWidth + lightUp * halfHeight;
	float solidAngle = RectangleSolidAngle(worldPos, p0, p1, p2, p3);

	if (dot(worldPos - light.positionWS, lightPlaneNormal) > 0)
	{
		fLight = solidAngle * 0.2 * (
			saturate(dot(normalize(p0 - worldPos), worldNormal)) +
			saturate(dot(normalize(p1 - worldPos), worldNormal)) +
			saturate(dot(normalize(p2 - worldPos), worldNormal)) +
			saturate(dot(normalize(p3 - worldPos), worldNormal)) +
			saturate(dot(normalize(light.positionWS - worldPos), worldNormal)));
	}
	fLight = max(0, fLight);


	// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
	{
		float3 r = reflect(-V, N);
		r = getSpecularDominantDirArea(N, r, roughness);

		float traced = Trace_rectangle(P, r, p0, p1, p2, p3);

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
			float3 tracedPlane = P + r * Trace_plane(P, r, light.positionWS, lightPlaneNormal);

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

			L = min - P;
		}
		L = normalize(L); // TODO: Is it necessary?
	}

	float3 lightColor = light.color.rgb*light.energy;

	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0) * fLight;
	result.diffuse = lightColor * fLight / PI;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);

	return result;
}
inline LightingResult TubeLight(in LightArrayType light, in float3 N, in float3 V, in float3 P, in float roughness, in float3 f0)
{
	LightingResult result = (LightingResult)0;

	float3 Lunormalized = light.positionWS - P;
	float dist = length(Lunormalized);
	float3 L = Lunormalized / dist;

	float sqrDist = dot(Lunormalized, Lunormalized);

	float3 lightLeft = _GetLeft(light);
	float lightWidth = _GetWidth(light);
	float3 worldPos = P;
	float3 worldNormal = N;


	float3 P0 = light.positionWS - lightLeft*lightWidth*0.5f;
	float3 P1 = light.positionWS + lightLeft*lightWidth*0.5f;

	// The sphere is placed at the nearest point on the segment. 
	// The rectangular plane is define by the following orthonormal frame: 
	float3 forward = normalize(ClosestPointOnLine(P0, P1, worldPos) - worldPos);
	float3 left = lightLeft;
	float3 up = cross(lightLeft, forward);

	float3 p0 = light.positionWS - left * (0.5 * lightWidth) + _GetRadius(light) * up;
	float3 p1 = light.positionWS - left * (0.5 * lightWidth) - _GetRadius(light) * up;
	float3 p2 = light.positionWS + left * (0.5 * lightWidth) - _GetRadius(light) * up;
	float3 p3 = light.positionWS + left * (0.5 * lightWidth) + _GetRadius(light) * up;


	float solidAngle = RectangleSolidAngle(worldPos, p0, p1, p2, p3);

	float fLight = solidAngle * 0.2 * (
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

	float fLightSphere = PI * saturate(dot(sphereL, worldNormal)) *
		((_GetRadius(light) * _GetRadius(light)) / sqrSphereDistance);

	fLight += fLightSphere;

	fLight = max(0, fLight);


	// We approximate L by the closest point on the reflection ray to the light source (representative point technique) to achieve a nice looking specular reflection
	{
		float3 r = reflect(V, N);
		r = getSpecularDominantDirArea(N, r, roughness);

		// First, the closest point to the ray on the segment
		float3 L0 = P0 - P;
		float3 L1 = P1 - P;
		float3 Ld = L1 - L0;

		float t = dot(r, L0) * dot(r, Ld) - dot(L0, Ld);
		t /= dot(Ld, Ld) - sqr(dot(r, Ld));

		L = (L0 + saturate(t) * Ld);

		// Then I place a sphere on that point and calculate the lisght vector like for sphere light.
		float3 centerToRay = dot(L, r) * r - L;
		float3 closestPoint = L + centerToRay * saturate(_GetRadius(light) / length(centerToRay));
		L = normalize(closestPoint);
	}

	float3 lightColor = light.color.rgb*light.energy;

	BRDF_MAKE(N, L, V);
	result.specular = lightColor * BRDF_SPECULAR(roughness, f0) * fLight;
	result.diffuse = lightColor * fLight / PI;

	result.diffuse = max(0.0f, result.diffuse);
	result.specular = max(0.0f, result.specular);

	return result;
}

#endif // _LIGHTING_HF_