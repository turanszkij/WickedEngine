#include "globals.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"
#include "ShaderInterop_TracedRendering.h"

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

Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	ray.energy = float3(1, 1, 1);
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
	float3 albedo;
	float3 specular;
	float emission;
};

RayHit CreateRayHit()
{
	RayHit hit;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.distance = 1.#INF;
	hit.normal = float3(0.0f, 0.0f, 0.0f);
	hit.emission = 0;
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
		bestHit.specular = float3(0.6, 0.6, 0.64);
		bestHit.albedo = float3(0.8f, 0.8f, 0.8f);
		bestHit.emission = 0;
	}
}

void IntersectSphere(Ray ray, inout RayHit bestHit, Sphere sphere)
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

RayHit Trace(Ray ray)
{
	RayHit bestHit = CreateRayHit();
	IntersectGroundPlane(ray, bestHit);
	
	const int dim = 2;
	const float dist = 2.5;
	for (int i = -dim; i <= dim; ++i)
	{
		for (int j = -dim; j <= dim; ++j)
		{
			Sphere sphere;
			sphere.position = float3(i * dist, 1, j * dist);
			sphere.radius = 1;
			sphere.specular = float3(0.04, 0.04, 0.04);
			sphere.albedo = float3(0.8f, 0.8f, 0.8f);
			sphere.emission = 0;

			if (i == 0)
			{
				sphere.emission = 2;
				sphere.position.y = 3;
			}

			IntersectSphere(ray, bestHit, sphere);
		}
	}

	return bestHit;
}

float sdot(float3 x, float3 y, float f = 1.0f)
{
	return saturate(dot(x, y) * f);
}

float rand(inout float seed, in float2 pixel)
{
	float result = frac(sin(seed * dot(pixel, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

float3x3 GetTangentSpace(float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = float3(1, 0, 0);
	if (abs(normal.x) > 0.99f)
		helper = float3(0, 0, 1);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

//float3 SampleHemisphere(float3 normal, inout float seed, in float2 pixel)
//{
//	// Uniformly sample hemisphere direction
//	float cosTheta = rand(seed, pixel);
//	float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
//	float phi = 2 * PI * rand(seed, pixel);
//	float3 tangentSpaceDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
//
//	// Transform direction to world space
//	return mul(tangentSpaceDir, GetTangentSpace(normal));
//}
float3 SampleHemisphere(float3 normal, float alpha, inout float seed, in float2 pixel)
{
	// Sample the hemisphere, where alpha determines the kind of the sampling
	float cosTheta = pow(rand(seed, pixel), 1.0f / (alpha + 1.0f));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = 2 * PI * rand(seed, pixel);
	float3 tangentSpaceDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	// Transform direction to world space
	return mul(tangentSpaceDir, GetTangentSpace(normal));
}

float energy(float3 color)
{
	return dot(color, 1.0f / 3.0f);
}

float3 Shade(inout Ray ray, RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < 1.#INF)
	{
		// Calculate chances of diffuse and specular reflection
		hit.albedo = min(1.0f - hit.specular, hit.albedo);
		float specChance = energy(hit.specular);
		float diffChance = energy(hit.albedo);
		float sum = specChance + diffChance;
		specChance /= sum;
		diffChance /= sum;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < specChance)
		{
			// Specular reflection
			float alpha = 150.0f;
			ray.origin = hit.position + hit.normal * 0.001f;
			ray.direction = SampleHemisphere(reflect(ray.direction, hit.normal), alpha, seed, pixel);
			float f = (alpha + 2) / (alpha + 1);
			ray.energy *= (1.0f / specChance) * hit.specular * sdot(hit.normal, ray.direction, f);


			//// Specular reflection
			//ray.origin = hit.position + hit.normal * 0.001f;
			//ray.direction = reflect(ray.direction, hit.normal);
			//ray.energy *= (1.0f / specChance) * hit.specular * sdot(hit.normal, ray.direction);
		}
		else
		{
			// Diffuse reflection
			ray.origin = hit.position + hit.normal * 0.001f;
			ray.direction = SampleHemisphere(hit.normal, 1.0f, seed, pixel);
			ray.energy *= (1.0f / diffChance) * hit.albedo;


			//// Diffuse reflection
			//ray.origin = hit.position + hit.normal * 0.001f;
			//ray.direction = SampleHemisphere(hit.normal, seed, pixel);
			//ray.energy *= (1.0f / diffChance) * 2 * hit.albedo * sdot(hit.normal, ray.direction);
		}

		return hit.emission;



		//// Phong shading
		//ray.origin = hit.position + hit.normal * 0.001f;
		//float3 reflected = reflect(ray.direction, hit.normal);
		//ray.direction = SampleHemisphere(hit.normal, seed, pixel);
		//float3 diffuse = 2 * min(1.0f - hit.specular, hit.albedo);
		//float alpha = 15.0f;
		//float3 specular = hit.specular * (alpha + 2) * pow(sdot(ray.direction, reflected), alpha);
		//ray.energy *= (diffuse + specular) * sdot(hit.normal, ray.direction);
		//return 0.0f;



		//ray.origin = hit.position + hit.normal * 0.001f;
		//ray.direction = SampleHemisphere(hit.normal, seed, pixel);
		//ray.energy *= 2 * hit.albedo * sdot(hit.normal, ray.direction);
		//return 0.0f;




		//float3 specular = float3(0.04, 0.04, 0.04);
		//float3 albedo = float3(0.8f, 0.8f, 0.8f);

		//// Reflect the ray and multiply energy with specular reflection
		//ray.origin = hit.position + hit.normal * 0.001f;
		//ray.direction = reflect(ray.direction, hit.normal);
		//ray.energy *= specular;

		//// Shadow test ray
		//bool shadow = false;
		//Ray shadowRay = CreateRay(hit.position + hit.normal * 0.001f, GetSunDirection());
		//RayHit shadowHit = Trace(shadowRay);
		//if (shadowHit.distance != 1.#INF)
		//{
		//	return float3(0.0f, 0.0f, 0.0f);
		//}

		//// Return a diffuse-shaded color
		//return saturate(dot(hit.normal, GetSunDirection())) * GetSunColor() * albedo;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		// Sample the skybox and write it
		float theta = acos(ray.direction.y) / -PI;
		float phi = atan2(ray.direction.x, -ray.direction.z) / -PI * 0.5f;
		//return _SkyboxTexture.SampleLevel(sampler_SkyboxTexture, float2(phi, theta), 0).xyz;
		return GetDynamicSkyColor(ray.direction);
	}
}


RWTEXTURE2D(Result, float4, 0);

[numthreads(TRACEDRENDERING_PRIMARY_BLOCKSIZE, TRACEDRENDERING_PRIMARY_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float seed = g_xFrame_Time;

	// Get the dimensions of the RenderTexture
	uint width, height;
	Result.GetDimensions(width, height);

	// Transform pixel to [-1,1] range
	float2 uv = float2((DTid.xy + float2(0.5f, 0.5f)) / float2(width, height) * 2.0f - 1.0f) * float2(1, -1);

	float3 finalResult = 0;
	uint rate = 0;

	[loop]
	for (; rate < 4; ++rate)
	{
		// Get a ray for the UVs
		Ray ray = CreateCameraRay(uv);

		// Trace and shade
		RayHit hit = Trace(ray);
		//float3 result = Shade(ray, hit);
		float3 result = float3(0, 0, 0);

		[loop]
		for (int i = 0; i < 4; i++)
		{
			RayHit hit = Trace(ray);
			result += ray.energy * Shade(ray, hit, seed, uv);

			if (!any(ray.energy))
				break;
		}

		finalResult += result;
	}
	finalResult /= rate;

	Result[DTid.xy] = float4(finalResult, 1);

	//Result[DTid.xy] = float4(ray.direction * 0.5f + 0.5f, 1.0f);
	//Result[DTid.xy] = float4(GetDynamicSkyColor(ray.direction), 1);
}