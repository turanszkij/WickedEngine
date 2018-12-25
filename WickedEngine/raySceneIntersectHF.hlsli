#ifndef _RAY_SCENE_INTERSECT_HF_
#define _RAY_SCENE_INTERSECT_HF_
#include "globals.hlsli"
#include "tracedRenderingHF.hlsli"

#ifndef RAYTRACE_STACKSIZE
#define RAYTRACE_STACKSIZE 32
#endif // RAYTRACE_STACKSIZE

STRUCTUREDBUFFER(materialBuffer, TracedRenderingMaterial, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(triangleBuffer, BVHMeshTriangle, TEXSLOT_ONDEMAND1);
RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(clusterOffsetBuffer, uint2, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(clusterConeBuffer, ClusterCone, TEXSLOT_ONDEMAND5);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND6);
STRUCTUREDBUFFER(bvhAABBBuffer, BVHAABB, TEXSLOT_ONDEMAND7);

TEXTURE2D(materialTextureAtlas, float4, TEXSLOT_ONDEMAND8);

inline RayHit TraceScene(Ray ray)
{
	RayHit bestHit = CreateRayHit();

	// Using BVH acceleration structure:

	// Emulated stack for tree traversal:
	uint stack[RAYTRACE_STACKSIZE];
	uint stackpos = 0;

	const uint clusterCount = clusterCounterBuffer.Load(0);
	const uint leafNodeOffset = clusterCount - 1;

	// push root node
	stack[stackpos] = 0;
	stackpos++;

	uint exit_condition = 0;
	do {
#ifdef RAYTRACE_EXIT
		if (exit_condition > RAYTRACE_EXIT)
			break;
		exit_condition++;
#endif // RAYTRACE_EXIT

		// pop untraversed node
		stackpos--;
		const uint nodeIndex = stack[stackpos];

		BVHNode node = bvhNodeBuffer[nodeIndex];
		BVHAABB box = bvhAABBBuffer[nodeIndex];

		if (IntersectBox(ray, box))
		{
			//if (node.LeftChildIndex == 0 && node.RightChildIndex == 0)
			if (nodeIndex >= clusterCount - 1)
			{
				// Leaf node
				const uint nodeToClusterID = nodeIndex - leafNodeOffset;
				const uint clusterIndex = clusterIndexBuffer[nodeToClusterID];
				bool cullCluster = false;

				//// Compute per cluster visibility:
				//const ClusterCone cone = clusterConeBuffer[clusterIndex];
				//if (cone.valid)
				//{
				//	const float3 testVec = normalize(ray.origin - cone.position);
				//	if (dot(testVec, cone.direction) > cone.angleCos)
				//	{
				//		cullCluster = true;
				//	}
				//}

				if (!cullCluster)
				{
					const uint2 cluster = clusterOffsetBuffer[clusterIndex];
					const uint triangleOffset = cluster.x;
					const uint triangleCount = cluster.y;

					for (uint tri = 0; tri < triangleCount; ++tri)
					{
						const uint primitiveID = triangleOffset + tri;
						IntersectTriangle(ray, bestHit, triangleBuffer[primitiveID], primitiveID);
					}
				}
			}
			else
			{
				// Internal node
				if (stackpos < RAYTRACE_STACKSIZE - 1)
				{
					// push left child
					stack[stackpos] = node.LeftChildIndex;
					stackpos++;
					// push right child
					stack[stackpos] = node.RightChildIndex;
					stackpos++;
				}
				else
				{
					// stack overflow, terminate
					break;
				}
			}

		}

	} while (stackpos > 0);


	return bestHit;
}

inline bool TraceSceneANY(Ray ray, float maxDistance)
{
	bool shadow = false;

	// Using BVH acceleration structure:

	// Emulated stack for tree traversal:
	uint stack[RAYTRACE_STACKSIZE];
	uint stackpos = 0;

	const uint clusterCount = clusterCounterBuffer.Load(0);
	const uint leafNodeOffset = clusterCount - 1;

	// push root node
	stack[stackpos] = 0;
	stackpos++;

	uint exit_condition = 0;
	do {
#ifdef RAYTRACE_EXIT
		if (exit_condition > RAYTRACE_EXIT)
			break;
		exit_condition++;
#endif // RAYTRACE_EXIT

		// pop untraversed node
		stackpos--;
		const uint nodeIndex = stack[stackpos];

		BVHNode node = bvhNodeBuffer[nodeIndex];
		BVHAABB box = bvhAABBBuffer[nodeIndex];

		if (IntersectBox(ray, box))
		{
			//if (node.LeftChildIndex == 0 && node.RightChildIndex == 0)
			if (nodeIndex >= clusterCount - 1)
			{
				// Leaf node
				const uint nodeToClusterID = nodeIndex - leafNodeOffset;
				const uint clusterIndex = clusterIndexBuffer[nodeToClusterID];
				bool cullCluster = false;

				//// Compute per cluster visibility:
				//const ClusterCone cone = clusterConeBuffer[clusterIndex];
				//if (cone.valid)
				//{
				//	const float3 testVec = normalize(ray.origin - cone.position);
				//	if (dot(testVec, cone.direction) > cone.angleCos)
				//	{
				//		cullCluster = true;
				//	}
				//}

				if (!cullCluster)
				{
					const uint2 cluster = clusterOffsetBuffer[clusterIndex];
					const uint triangleOffset = cluster.x;
					const uint triangleCount = cluster.y;

					for (uint tri = 0; tri < triangleCount; ++tri)
					{
						const uint primitiveID = triangleOffset + tri;
						if (IntersectTriangleANY(ray, maxDistance, triangleBuffer[primitiveID]))
						{
							shadow = true;
							break;
						}
					}
				}
			}
			else
			{
				// Internal node
				if (stackpos < RAYTRACE_STACKSIZE - 1)
				{
					// push left child
					stack[stackpos] = node.LeftChildIndex;
					stackpos++;
					// push right child
					stack[stackpos] = node.RightChildIndex;
					stackpos++;
				}
				else
				{
					// stack overflow, terminate
					break;
				}
			}

		}

	} while (!shadow && stackpos > 0);

	return shadow;
}

// This will modify ray to continue the trace
//	Also fill the final params of rayHit, such as normal, uv, materialIndex
//	seed should be > 0
//	pixel should be normalized uv coordinates of the ray start position (used to randomize)
inline float3 Shade(inout Ray ray, inout RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < INFINITE_RAYHIT)
	{
		BVHMeshTriangle tri = triangleBuffer[hit.primitiveID];

		float u = hit.bary.x;
		float v = hit.bary.y;
		float w = 1 - u - v;

		hit.N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		hit.UV = tri.t0 * w + tri.t1 * u + tri.t2 * v;
		hit.materialIndex = tri.materialIndex;

		TracedRenderingMaterial mat = materialBuffer[hit.materialIndex];

		hit.UV = frac(hit.UV); // emulate wrap
		float4 baseColorMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, hit.UV * mat.baseColorAtlasMulAdd.xy + mat.baseColorAtlasMulAdd.zw, 0);
		float4 surfaceMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, hit.UV * mat.surfaceMapAtlasMulAdd.xy + mat.surfaceMapAtlasMulAdd.zw, 0);
		float4 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, hit.UV * mat.normalMapAtlasMulAdd.xy + mat.normalMapAtlasMulAdd.zw, 0);

		float4 baseColor = DEGAMMA(mat.baseColor * baseColorMap);
		float reflectance = mat.reflectance * surfaceMap.r;
		float metalness = mat.metalness * surfaceMap.g;
		float3 emissive = baseColor.rgb * mat.emissive * surfaceMap.b;
		float roughness = mat.roughness * normalMap.a;
		roughness = sqr(roughness); // convert linear roughness to cone aperture
		float sss = mat.subsurfaceScattering;


		// Calculate chances of reflection types:
		float refractChance = 1 - baseColor.a;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < refractChance)
		{
			// Refraction
			float3 R = refract(ray.direction, hit.N, 1 - mat.refractionIndex);
			ray.direction = lerp(R, SampleHemisphere(R, seed, pixel), roughness);
			ray.energy *= lerp(baseColor.rgb, 1, refractChance);

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(hit.position, -hit.N);
		}
		else
		{
			// Calculate chances of reflection types:
			float3 albedo = ComputeAlbedo(baseColor, reflectance, metalness);
			float3 f0 = ComputeF0(baseColor, reflectance, metalness);
			float3 F = F_Fresnel(f0, saturate(dot(-ray.direction, hit.N)));
			float specChance = dot(F, 0.33);
			float diffChance = dot(albedo, 0.33);
			float inv = rcp(specChance + diffChance);
			specChance *= inv;
			diffChance *= inv;

			roulette = rand(seed, pixel);
			if (roulette < specChance)
			{
				// Specular reflection
				float3 R = reflect(ray.direction, hit.N);
				ray.direction = lerp(R, SampleHemisphere(R, seed, pixel), roughness);
				ray.energy *= F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.direction = SampleHemisphere(hit.N, seed, pixel);
				ray.energy *= albedo / diffChance;
			}

			// Ray reflects from surface, so push UP along normal to avoid self-intersection:
			ray.origin = trace_bias_position(hit.position, hit.N);
		}

		ray.primitiveID = hit.primitiveID;
		ray.bary = hit.bary;
		ray.Update();

		return emissive;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		return GetDynamicSkyColor(ray.direction);
	}
}

#endif // _RAY_SCENE_INTERSECT_HF_
