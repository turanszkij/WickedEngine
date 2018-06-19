#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RWTEXTURE2D(resultTexture, float4, 2);

STRUCTUREDBUFFER(triangleBuffer, TracedRenderingMeshTriangle, TEXSLOT_ONDEMAND1);
RAWBUFFER(clusterCounterBuffer, 0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(clusterOffsetBuffer, uint2, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(clusterConeBuffer, ClusterCone, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_UNIQUE0);
STRUCTUREDBUFFER(bvhAABBBuffer, TracedRenderingAABB, TEXSLOT_UNIQUE1);

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, StoredRay, TEXSLOT_ONDEMAND9);

inline bool TraceScene(Ray ray)
{
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

				// Compute per cluster visibility:
				const ClusterCone cone = clusterConeBuffer[clusterIndex];
				if (cone.valid)
				{
					const float3 testVec = normalize(ray.origin - cone.position);
					if (dot(testVec, cone.direction) > cone.angleCos)
					{
						cullCluster = true;
					}
				}

				if (!cullCluster)
				{
					const uint2 cluster = clusterOffsetBuffer[clusterIndex];
					const uint triangleOffset = cluster.x;
					const uint triangleCount = cluster.y;

					for (uint tri = 0; tri < triangleCount; ++tri)
					{
						bool hit = IntersectTriangleANY(ray, triangleBuffer[triangleOffset + tri]);

						if (hit)
						{
							return true;
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

	} while (stackpos > 0);


	return false;
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

#ifdef LDS_MESH // because that path has groupsync, every thread must go in following block
	}
	{
#endif // LDS_MESH

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);


		// Try dirlight:
		float3 N = ray.normal;
		float3 L = GetSunDirection();
		float NdotL = saturate(dot(L, N));

		if (NdotL > 0)
		{
			ray.direction = L;
			bool hit = TraceScene(ray);
			float3 result = (hit ? 0 : NdotL) * GetSunColor() * ray.energy;

			// Write pixel color:
			resultTexture[coords2D] += float4(result, 0);
		}
	}

	// This shader doesn't export any rays!
}