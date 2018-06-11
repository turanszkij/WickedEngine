#include "globals.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"
#include "ShaderInterop_TracedRendering.h"


RWTEXTURE2D(Result, float4, 0);

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND1);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND2);

struct Sphere
{
	float3 position;
	float radius;
	float3 albedo;
	float3 specular;
	float emission;
};

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
	float3 position;
	float distance;
	float3 normal;
	float3 albedo;
	float3 specular;
	float emission;
};

static const float INFINITE_RAYHIT = 1000000;
static const float EPSILON = 0.0001f;
inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.distance = INFINITE_RAYHIT;
	hit.normal = float3(0.0f, 0.0f, 0.0f);
	hit.emission = 0;
	return hit;
}

inline void IntersectGroundPlane(Ray ray, inout RayHit bestHit)
{
	// Calculate distance along the ray where the ground plane is intersected
	float t = -ray.origin.y / ray.direction.y;
	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.normal = float3(0.0f, 1.0f, 0.0f);
		bestHit.specular = float3(0.2, 0.2, 0.2);
		bestHit.albedo = float3(0.8f, 0.8f, 0.8f);
		bestHit.emission = 0;
	}
}

inline void IntersectSphere(Ray ray, inout RayHit bestHit, Sphere sphere)
{
	// Calculate distance along the ray where the sphere is intersected
	float3 d = ray.origin - sphere.position;
	float p1 = -dot(ray.direction, d);
	float p2sqr = p1 * p1 - dot(d, d) + sphere.radius * sphere.radius;
	if (p2sqr < 0)
		return;
	float p2 = sqrt(p2sqr);
	float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.normal = normalize(bestHit.position - sphere.position);
		bestHit.albedo = sphere.albedo;
		bestHit.specular = sphere.specular;
		bestHit.emission = sphere.emission;
	}
}


struct Triangle
{
	float3 v0, v1, v2;
	float3 n0, n1, n2;
	float2 t0, t1, t2;
	uint materialIndex;
};

#define CULLING
inline void IntersectTriangle(Ray ray, inout RayHit bestHit, in Triangle tri)
{
	float3 v0v1 = tri.v1 - tri.v0;
	float3 v0v2 = tri.v2 - tri.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det > EPSILON)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0;
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
		bestHit.normal = normalize(cross(tri.v2 - tri.v1, tri.v1 - tri.v0));
		bestHit.albedo = 0.9;
		bestHit.specular = 0.1;
		bestHit.emission = 0;

		if (tri.materialIndex == 1) bestHit.albedo = float3(0, 0.9, 0);
		if (tri.materialIndex == 2) bestHit.albedo = float3(0.9, 0, 0);

		float2 uv = float2(u, v);
	}
}

inline RayHit TraceScene(Ray ray)
{
	RayHit bestHit = CreateRayHit();


	//IntersectGroundPlane(ray, bestHit);
	
	//const int dim = 2;
	//const float dist = 2.5;
	//for (int i = -dim; i <= dim; ++i)
	//{
	//	for (int j = -dim; j <= dim; ++j)
	//	{
	//		Sphere sphere;
	//		sphere.position = float3(i * dist, 1, j * dist);
	//		sphere.radius = 1;
	//		sphere.specular = float3(0.04, 0.04, 0.04);
	//		sphere.albedo = float3(0.8f, 0.8f, 0.8f);
	//		sphere.emission = 0;

	//		if (i == 0)
	//		{
	//			sphere.emission = 2;
	//			sphere.position.y = 3;

	//			if (j == 0)
	//			{
	//				sphere.position = float3(100, 200, 100);
	//				sphere.radius = 100;
	//				sphere.emission = 4;
	//				sphere.albedo = float3(0.9, 0.9, 0.2);
	//			}
	//		}

	//		if (i + j == dim)
	//		{
	//			sphere.albedo = float3(0.7, 0.9, 0.2);
	//			sphere.specular = 0;
	//			sphere.emission = 1;
	//		}

	//		IntersectSphere(ray, bestHit, sphere);
	//	}
	//}


	Sphere sphere;
	sphere.position = float3(8, 20, 25);
	sphere.radius = 4;
	sphere.specular = 0;
	sphere.albedo = float3(1, 0.5, 0.1);
	sphere.emission = 4;
	IntersectSphere(ray, bestHit, sphere);

	for (uint tri = 0; tri < xTraceMeshTriangleCount; ++tri)
	{
		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 1];
		uint i2 = meshIndexBuffer[tri * 3 + 2];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * 16));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * 16));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * 16));

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

		Triangle tri;
		tri.v0 = pos_nor0.xyz;
		tri.v1 = pos_nor1.xyz;
		tri.v2 = pos_nor2.xyz;
		tri.n0 = nor0;
		tri.n1 = nor1;
		tri.n2 = nor2;
		tri.t0 = meshVertexBuffer_TEX[i0];
		tri.t1 = meshVertexBuffer_TEX[i1];
		tri.t2 = meshVertexBuffer_TEX[i2];
		tri.materialIndex = materialIndex;

		IntersectTriangle(ray, bestHit, tri);
	}

	//IntersectTriangle(ray, bestHit, float3(0,0,0), float3(10,0,0), float3(10,10,0));


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
	float3 tangent = normalize(cross(normal, g_xFrame_MainCamera_At));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
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
		// Calculate chances of diffuse and specular reflection
		hit.albedo = min(1.0f - hit.specular, hit.albedo);
		float specChance = dot(hit.specular, 0.33);
		float diffChance = dot(hit.albedo, 0.33);
		float inv = 1.0f / (specChance + diffChance);
		specChance *= inv;
		diffChance *= inv;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < specChance)
		{
			// Specular reflection
			float alpha = 150.0f;
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(reflect(ray.direction, hit.normal), alpha, seed, pixel);
			float f = (alpha + 2) / (alpha + 1);
			ray.energy *= (1.0f / specChance) * hit.specular * saturate(dot(hit.normal, ray.direction) * f);
		}
		else
		{
			// Diffuse reflection
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(hit.normal, 1.0f, seed, pixel);
			ray.energy *= (1.0f / diffChance) * hit.albedo;
		}

		return hit.emission;
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
