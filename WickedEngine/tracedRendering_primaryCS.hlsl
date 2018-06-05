#include "globals.hlsli"
#include "skyHF.hlsli"
#include "ShaderInterop_TracedRendering.h"

struct Ray
{
	float3 origin;
	float3 direction;
};

Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	return ray;
}

Ray CreateCameraRay(float2 uv)
{
	// Transform the camera origin to world space
	float3 origin = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), g_xFrame_MainCamera_InvV).xyz;

	// Invert the perspective projection of the view-space position
	float3 direction = mul(float4(uv, 0.0f, 1.0f), g_xFrame_MainCamera_InvP).xyz;
	// Transform the direction from camera to world space and normalize
	direction = mul(float4(direction, 0.0f), g_xFrame_MainCamera_InvV).xyz;
	direction = normalize(direction);

	return CreateRay(origin, direction);
}

struct RayHit
{
	float3 position;
	float distance;
	float3 normal;
};

RayHit CreateRayHit()
{
	RayHit hit;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.distance = 1.#INF;
	hit.normal = float3(0.0f, 0.0f, 0.0f);
	return hit;
}

void IntersectGroundPlane(Ray ray, inout RayHit bestHit)
{
	// Calculate distance along the ray where the ground plane is intersected
	float t = -ray.origin.y / ray.direction.y;
	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.normal = float3(0.0f, 1.0f, 0.0f);
	}
}

void IntersectSphere(Ray ray, inout RayHit bestHit, float4 sphere)
{
	// Calculate distance along the ray where the sphere is intersected
	float3 d = ray.origin - sphere.xyz;
	float p1 = -dot(ray.direction, d);
	float p2sqr = p1 * p1 - dot(d, d) + sphere.w * sphere.w;
	if (p2sqr < 0)
		return;
	float p2 = sqrt(p2sqr);
	float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.normal = normalize(bestHit.position - sphere.xyz);
	}
}

RayHit Trace(Ray ray)
{
	RayHit bestHit = CreateRayHit();
	IntersectGroundPlane(ray, bestHit);
	
	// Add a floating unit sphere
	IntersectSphere(ray, bestHit, float4(0, 3.0f, 0, 1.0f));

	return bestHit;
}


float3 Shade(inout Ray ray, RayHit hit)
{
	if (hit.distance < 1.#INF)
	{
		// Return the normal
		return hit.normal * 0.5f + 0.5f;
	}
	else
	{
		// Sample the skybox and write it
		float theta = acos(ray.direction.y) / -PI;
		float phi = atan2(ray.direction.x, -ray.direction.z) / -PI * 0.5f;
		return GetDynamicSkyColor(ray.direction);
	}
}


RWTEXTURE2D(Result, float4, 0);

[numthreads(TRACEDRENDERING_PRIMARY_BLOCKSIZE, TRACEDRENDERING_PRIMARY_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Get the dimensions of the RenderTexture
	uint width, height;
	Result.GetDimensions(width, height);

	// Transform pixel to [-1,1] range
	float2 uv = float2((DTid.xy + float2(0.5f, 0.5f)) / float2(width, height) * 2.0f - 1.0f) * float2(1, -1);

	// Get a ray for the UVs
	Ray ray = CreateCameraRay(uv);

	// Trace and shade
	RayHit hit = Trace(ray);
	float3 result = Shade(ray, hit);
	Result[DTid.xy] = float4(result, 1);

	//Result[DTid.xy] = float4(ray.direction * 0.5f + 0.5f, 1.0f);
	//Result[DTid.xy] = float4(GetDynamicSkyColor(ray.direction), 1);
}