#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_Renderer.h"

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND6);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	const float2 bluenoise = blue_noise(unflatten2D(DTid.x, 256)).xy;

	Surfel surfel = surfelBuffer[DTid.x];

	float seed = surfel.life;
	float2 uv = float2(g_xFrame_Time, (float)DTid.x / (float)surfel_count);

	Surface surface;
	surface.init();
	surface.N = normalize(unpack_unitvector(surfel.normal));

	RayDesc ray;
	ray.Origin = surfel.position;
	//ray.Direction = normalize(mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), GetTangentSpace(surface.N)));
	ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);
	ray.TMin = 0.001;
	ray.TMax = FLT_MAX;

	float3 result = 0;
	float3 energy = 1;

	uint bounces = 2;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(energy)); ++bounce)
	{
		surface.P = ray.Origin;

		// Sample primary ray (scene materials, sky, etc):
		ray.Direction = normalize(ray.Direction);

#ifdef RTAPI
		RayQuery<
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
		> q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
#ifdef RAY_BACKFACE_CULLING
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
#endif // RAY_BACKFACE_CULLING
			RAY_FLAG_FORCE_OPAQUE |
			0,								// uint RayFlags
			0xFF,							// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray);

		if (hit.distance >= FLT_MAX - 1)
#endif // RTAPI

		{
			float3 envColor;
			[branch]
			if (IsStaticSky())
			{
				// We have envmap information in a texture:
				envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.Direction, 0).rgb);
			}
			else
			{
				envColor = GetDynamicSkyColor(ray.Direction, true, true, false, true);
			}
			result += max(0, energy * envColor);

			// Erase the ray's energy
			energy = 0;
			break;
		}

		ShaderMaterial material;

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(q.CommittedInstanceID())].Load<ShaderMesh>(0);
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][q.CommittedGeometryIndex()];
		material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);

		EvaluateObjectSurface(
			mesh,
			subset,
			material,
			q.CommittedPrimitiveIndex(),
			q.CommittedTriangleBarycentrics(),
			q.CommittedObjectToWorld3x4(),
			surface
		);

#else
		// ray origin updated for next bounce:
		ray.Origin = ray.Origin + ray.Direction * hit.distance;

		EvaluateObjectSurface(
			hit,
			material,
			surface
		);

#endif // RTAPI

		surface.update();

		// Calculate chances of reflection types:
		const float refractChance = surface.transmission;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.Direction, surface.N, 1 - material.refraction);
			ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
			energy *= surface.albedo;

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;
		}
		else
		{
			// Calculate chances of reflection types:
			const float specChance = dot(surface.F, 0.333f);

			roulette = rand(seed, uv);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.Direction, surface.N);
				ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
				energy *= surface.F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);
				energy *= surface.albedo / (1 - specChance);
			}
		}

		result += max(0, energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

	}

	float blendfactor = lerp(1, 0.01, saturate(surfel.life * 10));
	surfel.color = lerp(surfel.color, result, blendfactor);
	surfel.life += g_xFrame_DeltaTime;
	surfelBuffer[DTid.x] = surfel;
}
