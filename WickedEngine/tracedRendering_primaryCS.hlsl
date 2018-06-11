#include "globals.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"
#include "ShaderInterop_TracedRendering.h"


RWTEXTURE2D(Result, float4, 0);

struct Material
{
	float4		baseColor;
	float4		texMulAdd;
	float		roughness;
	float		reflectance;
	float		metalness;
	float		emissive;
	float		refractionIndex;
	float		subsurfaceScattering;
	float		normalMapStrength;
	float		parallaxOcclusionMapping;
};
STRUCTUREDBUFFER(materialBuffer, Material, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND3);

TEXTURE2D(texture_baseColor, float4, TEXSLOT_ONDEMAND4);
TEXTURE2D(texture_normalMap, float4, TEXSLOT_ONDEMAND5);
TEXTURE2D(texture_surfaceMap, float4, TEXSLOT_ONDEMAND6);

//struct Sphere
//{
//	float3 position;
//	float radius;
//	float3 albedo;
//	float3 specular;
//	float emission;
//};

struct Ray
{
	float3 origin;
	float3 direction;
	float3 energy;
};

inline Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	ray.energy = float3(1, 1, 1);
	return ray;
}

inline Ray CreateCameraRay(float2 uv)
{
	// Invert the perspective projection of the view-space position
	float3 direction = mul(float4(uv, 0.0f, 1.0f), g_xFrame_MainCamera_InvP).xyz;
	// Transform the direction from camera to world space and normalize
	direction = mul(float4(direction, 0.0f), g_xFrame_MainCamera_InvV).xyz;
	direction = normalize(direction);

	return CreateRay(g_xFrame_MainCamera_CamPos, direction);
}

struct RayHit
{
	float distance;
	float3 position;
	float3 normal;
	uint materialIndex;
	float2 texCoords;

	//float3 albedo;
	//float3 specular;
	//float emission;
};

static const float INFINITE_RAYHIT = 1000000;
static const float EPSILON = 0.0001f;
inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.distance = INFINITE_RAYHIT;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.normal = float3(0.0f, 0.0f, 0.0f);
	hit.materialIndex = 0;
	hit.texCoords = 0;

	return hit;
}

//inline void IntersectGroundPlane(Ray ray, inout RayHit bestHit)
//{
//	// Calculate distance along the ray where the ground plane is intersected
//	float t = -ray.origin.y / ray.direction.y;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = float3(0.0f, 1.0f, 0.0f);
//		bestHit.specular = float3(0.2, 0.2, 0.2);
//		bestHit.albedo = float3(0.8f, 0.8f, 0.8f);
//		bestHit.emission = 0;
//	}
//}
//
//inline void IntersectSphere(Ray ray, inout RayHit bestHit, Sphere sphere)
//{
//	// Calculate distance along the ray where the sphere is intersected
//	float3 d = ray.origin - sphere.position;
//	float p1 = -dot(ray.direction, d);
//	float p2sqr = p1 * p1 - dot(d, d) + sphere.radius * sphere.radius;
//	if (p2sqr < 0)
//		return;
//	float p2 = sqrt(p2sqr);
//	float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = normalize(bestHit.position - sphere.position);
//		bestHit.albedo = sphere.albedo;
//		bestHit.specular = sphere.specular;
//		bestHit.emission = sphere.emission;
//	}
//}


struct Triangle
{
	float3 v0, v1, v2;
	float3 n0, n1, n2;
	float2 t0, t1, t2;
	uint materialIndex;
};

#define CULLING
inline void IntersectTriangle(Ray ray, inout RayHit bestHit, in Triangle primitive)
{
	float3 v0v1 = primitive.v1 - primitive.v0;
	float3 v0v2 = primitive.v2 - primitive.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < EPSILON)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - primitive.v0;
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1) 
		return;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.direction, qvec) * invDet;
	if (v < 0 || u + v > 1) 
		return;

	float t = dot(v0v2, qvec) * invDet;

	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;

		float w = 1 - u - v;
		bestHit.normal = primitive.n0 * w + primitive.n1 * u + primitive.n2 * v;
		bestHit.texCoords = primitive.t0 * w + primitive.t1 * u + primitive.t2 * v;

		bestHit.materialIndex = primitive.materialIndex;
	}
}

inline RayHit TraceScene(Ray ray)
{
	RayHit bestHit = CreateRayHit();

	for (uint tri = 0; tri < xTraceMeshTriangleCount; ++tri)
	{
		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 2];
		uint i2 = meshIndexBuffer[tri * 3 + 1];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xTraceMeshVertexPOSStride));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xTraceMeshVertexPOSStride));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xTraceMeshVertexPOSStride));

		uint nor_u = asuint(pos_nor0.w);
		uint materialIndex;
		float3 nor0;
		{
			nor0.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			materialIndex = (nor_u >> 28) & 0x0000000F;
		}
		nor_u = asuint(pos_nor1.w);
		float3 nor1;
		{
			nor1.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = asuint(pos_nor2.w);
		float3 nor2;
		{
			nor2.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}

		Triangle primitive;
		primitive.v0 = pos_nor0.xyz;
		primitive.v1 = pos_nor1.xyz;
		primitive.v2 = pos_nor2.xyz;
		primitive.n0 = nor0;
		primitive.n1 = nor1;
		primitive.n2 = nor2;
		primitive.t0 = meshVertexBuffer_TEX[i0];
		primitive.t1 = meshVertexBuffer_TEX[i1];
		primitive.t2 = meshVertexBuffer_TEX[i2];
		primitive.materialIndex = materialIndex;

		IntersectTriangle(ray, bestHit, primitive);
	}


	return bestHit;
}

inline float rand(inout float seed, in float2 pixel)
{
	float result = frac(sin(seed * dot(pixel, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

inline float3x3 GetTangentSpace(float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99f ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);

	//float3 tangent = normalize(cross(normal, g_xFrame_MainCamera_At));
	//float3 binormal = normalize(cross(normal, tangent));
	//return float3x3(tangent, binormal, normal);
}

inline float3 SampleHemisphere(float3 normal, float alpha, inout float seed, in float2 pixel)
{
	// Sample the hemisphere, where alpha determines the kind of the sampling
	float cosTheta = pow(rand(seed, pixel), 1.0f / (alpha + 1.0f));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = 2 * PI * rand(seed, pixel);
	float3 tangentSpaceDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	// Transform direction to world space
	return mul(tangentSpaceDir, GetTangentSpace(normal));
}

inline float3 Shade(inout Ray ray, RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < INFINITE_RAYHIT)
	{
		float4 baseColorMap = texture_baseColor.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);
		float4 normalMap = texture_normalMap.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);
		float4 surfaceMap = texture_surfaceMap.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);

		Material mat = materialBuffer[hit.materialIndex];
		
		float4 baseColor = mat.baseColor * baseColorMap;
		float reflectance = mat.reflectance/* * surfaceMap.r*/;
		float metalness = mat.metalness/* * surfaceMap.g*/;
		float3 emissive = baseColor.rgb * mat.emissive * surfaceMap.b;
		float roughness = mat.roughness/* * normalMap.a*/;

		float3 albedo = ComputeAlbedo(baseColor, reflectance, metalness);
		float3 specular = ComputeF0(baseColor, reflectance, metalness);


		// Calculate chances of diffuse and specular reflection
		albedo = min(1.0f - specular, albedo);
		float specChance = dot(specular, 0.33);
		float diffChance = dot(albedo, 0.33);
		float inv = 1.0f / (specChance + diffChance);
		specChance *= inv;
		diffChance *= inv;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < specChance)
		{
			// Specular reflection
			//float alpha = 150.0f;
			float alpha = sqr(1 - roughness) * 1000;
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(reflect(ray.direction, hit.normal), alpha, seed, pixel);
			float f = (alpha + 2) / (alpha + 1);
			ray.energy *= (1.0f / specChance) * specular * saturate(dot(hit.normal, ray.direction) * f);
		}
		else
		{
			// Diffuse reflection
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(hit.normal, 1.0f, seed, pixel);
			ray.energy *= (1.0f / diffChance) * albedo;
		}

		return emissive;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		return GetDynamicSkyColor(ray.direction);
	}
}

[numthreads(TRACEDRENDERING_PRIMARY_BLOCKSIZE, TRACEDRENDERING_PRIMARY_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float seed = g_xFrame_Time;

	// Compute screen coordinates:
	float2 uv = float2((DTid.xy + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

	// Create starting ray:
	Ray ray = CreateCameraRay(uv);


	// Trace the scene with a limited ray bounces:
	const int maxBounces = 8;
	float3 result = float3(0, 0, 0);
	for (int i = 0; i < maxBounces; i++)
	{
		RayHit hit = TraceScene(ray);
		result += ray.energy * Shade(ray, hit, seed, uv);

		if (!any(ray.energy))
			break;
	}


	// Write the result:

	//Result[DTid.xy] = float4(result, 1);

	// Accumulate history:
	uint sam = xTraceSample;
	float alpha = 1.0f / (sam + 1.0f);
	float3 old = Result[DTid.xy].rgb;
	Result[DTid.xy] = float4(old * (1-alpha) + result * alpha, 1);
	//Result[DTid.xy] = 0;
}
