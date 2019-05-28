#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader iterates through all clusters and performs the following:
//	- Copies the morton codes to the sorted index buffer order. Morton codes can be used after this to assemble the BVH.
//	- Computes cluster visibility cone. Points inside the cone only see cluster backfaces, so they can be culled early.
//		Source: https://gpuopen.com/geometryfx-1-2-cluster-culling/ and https://github.com/GPUOpen-Effects/GeometryFX

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(clusterMortonBuffer, float, TEXSLOT_ONDEMAND2); // morton buffer is float because sorting is written for floats!
STRUCTUREDBUFFER(clusterOffsetBuffer, uint2, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(clusterAABBBuffer, BVHAABB, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(triangleBuffer, BVHMeshTriangle, TEXSLOT_ONDEMAND5);

RWSTRUCTUREDBUFFER(clusterSortedMortonBuffer, float, 0); // morton buffer is float because sorting is written for floats!
RWSTRUCTUREDBUFFER(clusterConeBuffer, ClusterCone, 1);

[numthreads(BVH_CLUSTERPROCESSOR_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < clusterCounterBuffer.Load(0))
	{
		const uint clusterIndex = clusterIndexBuffer[DTid.x];

		// 1.) Sorted morton ordered copy:
		clusterSortedMortonBuffer[DTid.x] = clusterMortonBuffer[clusterIndex];

		// 2.) Visiblity cone:
		const uint2 cluster = clusterOffsetBuffer[clusterIndex];
		const uint triangleOffset = cluster.x;
		const uint triangleCount = cluster.y;

		// Center point of cluster AABB:
		const BVHAABB aabb = clusterAABBBuffer[clusterIndex];
		const float3 center = (aabb.min + aabb.max) * 0.5f;

		ClusterCone cone;
		cone.valid = 1;
		cone.position = 0;
		cone.direction = 0;
		cone.angleCos = 0;

		// Iterate through triangles to find cone direction:
		{
			for (uint tri = 0; tri < triangleCount; ++tri)
			{
				BVHMeshTriangle prim = triangleBuffer[triangleOffset + tri];
				float3 facenormal = cross(prim.v1 - prim.v0, prim.v2 - prim.v0);

				// direction will be sum of negative face normals:
				cone.direction += -facenormal;
			}
			cone.direction = normalize(cone.direction);
		}

		// Find cone center and angle:
		//	We nee a second pass to find the intersection of the line
		//	center + t * coneAxis with the plane defined by each triangle
		{
			float t = -1000000;
			float coneOpening = 1;
			for (uint tri = 0; tri < triangleCount; ++tri)
			{
				BVHMeshTriangle prim = triangleBuffer[triangleOffset + tri];
				float3 facenormal = cross(prim.v1 - prim.v0, prim.v2 - prim.v0);

				const float directionalPart = dot(cone.direction, -facenormal);
				if (directionalPart < 0)
				{
					// No solution for this cluster - at least two triangles are facing each other
					cone.valid = 0;
					break;
				}

				const float td = dot(center - prim.v0, facenormal) / -directionalPart;

				t = max(t, td);
				coneOpening = min(coneOpening, directionalPart);
			}

			// cos (PI/2 - acos (coneOpening))
			cone.angleCos = sqrt(1 - coneOpening * coneOpening);
			cone.position = center + cone.direction * t;
		}

		// Cluster center safety check:
		//	If distance of coneCenter to the bounding box center is more
		//	than 16x the bounding box extent, the cluster is also invalid
		//	This is mostly a safety measure - if triangles are nearly
		//	parallel to coneAxis, t may become very large and unstable
		{
			const float aabbSize = length(aabb.max - aabb.min);
			const float coneCenterToCenterDistance = length(cone.position - center);
			if (coneCenterToCenterDistance > (16 * aabbSize))
			{
				cone.valid = 0;
			}
		}


		// Store cluster cone:
		clusterConeBuffer[clusterIndex] = cone;
	}
}
