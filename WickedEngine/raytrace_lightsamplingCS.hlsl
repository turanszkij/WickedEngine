#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RWTEXTURE2D(resultTexture, float4, 0);

STRUCTUREDBUFFER(triangleBuffer, TracedRenderingMeshTriangle, TEXSLOT_ONDEMAND1);
RAWBUFFER(clusterCounterBuffer, 0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(clusterOffsetBuffer, uint2, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(clusterConeBuffer, ClusterCone, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_UNIQUE0);
STRUCTUREDBUFFER(bvhAABBBuffer, TracedRenderingAABB, TEXSLOT_UNIQUE1);

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, StoredRay, TEXSLOT_ONDEMAND9);

inline bool TraceSceneANY(Ray ray, float maxDistance)
{
	bool shadow = false;

	// Using BVH acceleration structure:

	// Emulated stack for tree traversal:
	const uint stacksize = 32;
	uint stack[stacksize];
	uint stackpos = 0;

	const uint clusterCount = clusterCounterBuffer.Load(0);
	const uint leafNodeOffset = clusterCount - 1;

	// push root node
	stack[stackpos] = 0;
	stackpos++;

	do {
		// pop untraversed node
		stackpos--;
		const uint nodeIndex = stack[stackpos];

		BVHNode node = bvhNodeBuffer[nodeIndex];
		TracedRenderingAABB box = bvhAABBBuffer[nodeIndex];

		if (IntersectBox(ray, box))
		{
			if (node.LeftChildIndex == 0 && node.RightChildIndex == 0)
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
						if (IntersectTriangleANY(ray, maxDistance, triangleBuffer[triangleOffset + tri]))
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
				if (stackpos < stacksize - 1)
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

[numthreads(TRACEDRENDERING_TRACE_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	// Initialize ray and pixel ID as non-contributing:
	Ray ray = (Ray)0;
	uint pixelID = 0xFFFFFFFF;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		LoadRay(rayBuffer_READ[DTid.x], ray, pixelID);

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

		float seed = g_xFrame_Time + xTraceBounce;

		float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;

		float3 finalResult = 0;

		float3 P = ray.origin;
		float3 N = ray.normal;
		float3 V = normalize(g_xFrame_MainCamera_CamPos - P);

		float4 baseColor = 1;
		float roughness = 0.2f;
		float reflectance = 0.3f;
		float metalness = 0;
		float emissive = 0;
		float sss = 0;
		Surface surface = CreateSurface(P, N, V, baseColor, roughness, reflectance, metalness, emissive, sss);

		[loop]
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntityType light = EntityArray[g_xFrame_LightArrayOffset + iterator];

			LightingResult result = (LightingResult)0;
		
			float3 L = 0;
			float dist = 0;
		
			switch (light.type)
			{
			case ENTITY_TYPE_DIRECTIONALLIGHT:
			{
				dist = INFINITE_RAYHIT;

				float3 lightColor = light.GetColor().rgb*light.energy;

				L = light.directionWS.xyz;
				SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

				result.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
				result.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);
			}
			break;
			case ENTITY_TYPE_POINTLIGHT:
			{
				L = light.positionWS - P;
				dist = length(L);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					float3 lightColor = light.GetColor().rgb*light.energy;

					SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

					result.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
					result.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);

					float att = (light.energy * (light.range / (light.range + 1 + dist)));
					float attenuation = (att * (light.range - dist) / light.range);
					result.diffuse *= attenuation;
					result.specular *= attenuation;
				}
			}
			break;
			case ENTITY_TYPE_SPOTLIGHT:
			{
				L = light.positionWS - surface.P;
				dist = length(L);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					float3 lightColor = light.GetColor().rgb*light.energy;

					float SpotFactor = dot(L, light.directionWS);
					float spotCutOff = light.coneAngleCos;

					[branch]
					if (SpotFactor > spotCutOff)
					{
						SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

						result.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
						result.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);

						float att = (light.energy * (light.range / (light.range + 1 + dist)));
						float attenuation = (att * (light.range - dist) / light.range);
						attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));
						result.diffuse *= attenuation;
						result.specular *= attenuation;
					}
				}
			}
			break;
			case ENTITY_TYPE_SPHERELIGHT:
			{
			}
			break;
			case ENTITY_TYPE_DISCLIGHT:
			{
			}
			break;
			case ENTITY_TYPE_RECTANGLELIGHT:
			{
			}
			break;
			case ENTITY_TYPE_TUBELIGHT:
			{
			}
			break;
			}
		
			float NdotL = saturate(dot(L, N));
		
			if (NdotL > 0 && dist > 0)
			{
				result.diffuse = max(0.0f, result.diffuse);
				result.specular = max(0.0f, result.specular);

				Ray newRay;
				newRay.origin = P + N * EPSILON;
				newRay.direction = L + sampling_offset * 0.025f;
				newRay.direction_inverse = rcp(newRay.direction);
				newRay.energy = 0;
				bool hit = TraceSceneANY(newRay, dist);
				finalResult += (hit ? 0 : NdotL) * (result.diffuse + result.specular);
			}
		}
		
		finalResult *= ray.energy;

		resultTexture[coords2D] += float4(max(0, finalResult), 0);

	}

	// This shader doesn't export any rays!
}