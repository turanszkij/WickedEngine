#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Patch, 0);
RWSTRUCTUREDBUFFER(simulationBuffer, PatchSimulationData, 1);

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND1);

#define NUM_LDS_FORCEFIELDS 32
struct LDS_ForceField
{
	uint type;
	float3 position;
	float gravity;
	float range_inverse;
	float3 normal;
};
groupshared LDS_ForceField forceFields[NUM_LDS_FORCEFIELDS];

[numthreads(THREADCOUNT_SIMULATEHAIR, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	// Load the forcefields into LDS:
	uint numForceFields = min(g_xFrame_ForceFieldArrayCount, NUM_LDS_FORCEFIELDS);
	if (groupIndex < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldArrayOffset + groupIndex;
		ShaderEntityType forceField = EntityArray[forceFieldID];

		forceFields[groupIndex].type = (uint)forceField.GetType();
		forceFields[groupIndex].position = forceField.positionWS;
		forceFields[groupIndex].gravity = forceField.energy;
		forceFields[groupIndex].range_inverse = forceField.range;
		forceFields[groupIndex].normal = forceField.directionWS;
	}


	// Generate patch:
	float2 uv = float2(Gid.x / (float)xHairNumDispatchGroups, (float)DTid.x / (float)THREADCOUNT_SIMULATEHAIR);
	float seed = xHairRandomSeed;

	// random triangle on emitter surface:
	uint tri = (uint)((xHairBaseMeshIndexCount / 3) * rand(seed, uv));

	// load indices of triangle from index buffer
	uint i0 = meshIndexBuffer[tri * 3 + 0];
	uint i1 = meshIndexBuffer[tri * 3 + 1];
	uint i2 = meshIndexBuffer[tri * 3 + 2];

	// load vertices of triangle from vertex buffer:
	float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xHairBaseMeshVertexPositionStride));

	uint nor_u = asuint(pos_nor0.w);
	float3 nor0;
	{
		nor0.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor0.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		nor0.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
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

	// random barycentric coords:
	float f = rand(seed, uv);
	float g = rand(seed, uv);
	[flatten]
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}

	// compute final surface position on triangle from barycentric coords:
	float3 position = pos_nor0.xyz + f * (pos_nor1.xyz - pos_nor0.xyz) + g * (pos_nor2.xyz - pos_nor0.xyz);
	float3 target = normalize(nor0 + f * (nor1 - nor0) + g * (nor2 - nor0));
	float3 tangent = normalize((rand(seed, uv) < 0.5f ? pos_nor0.xyz : pos_nor2.xyz) - pos_nor1.xyz);
	float3 binormal = cross(target, tangent);

	uint tangent_random = 0;
	tangent_random |= (uint)((uint)(tangent.x * 127.5f + 127.5f) << 0);
	tangent_random |= (uint)((uint)(tangent.y * 127.5f + 127.5f) << 8);
	tangent_random |= (uint)((uint)(tangent.z * 127.5f + 127.5f) << 16);
	tangent_random |= (uint)(rand(seed, uv) * 255) << 24;

	uint binormal_length = 0;
	binormal_length |= (uint)((uint)(binormal.x * 127.5f + 127.5f) << 0);
	binormal_length |= (uint)((uint)(binormal.y * 127.5f + 127.5f) << 8);
	binormal_length |= (uint)((uint)(binormal.z * 127.5f + 127.5f) << 16);
	binormal_length |= (uint)(rand(seed, uv) * 255 * saturate(xHairRandomness)) << 24;

	// Identifies the hair strand root particle:
	const uint strandID = DTid.x * xHairSegmentCount;

	float3 segmentRootNormal = 0;
	for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
	{
		// Identifies the hair strand segment particle:
		const uint particleID = strandID + segmentID;
		particleBuffer[particleID].tangent_random = tangent_random;
		particleBuffer[particleID].binormal_length = binormal_length;

		[branch]
		if (xHairRegenerate)
		{
			particleBuffer[particleID].normal = target;
		}

		float3 normal = particleBuffer[particleID].normal;
		normal += segmentRootNormal;

		// Particle buffer load:
		float len = (binormal_length >> 24) & 0x000000FF;
		len /= 255.0f;
		len += 1;
		len *= xLength;

		// Simulation buffer load:
		float3 velocity = simulationBuffer[particleID].velocity;

		// transform particle by the emitter object matrix:
		normal = normalize(mul((float3x3)xWorld, normal));
		target = normalize(mul((float3x3)xWorld, target));

		float3 tip = mul(xWorld, float4(position.xyz, 1)).xyz + normal * len;

		// Accumulate forces:
		GroupMemoryBarrierWithGroupSync();
		float3 force = 0;
		for (uint i = 0; i < numForceFields; ++i)
		{
			LDS_ForceField forceField = forceFields[i];

			float3 dir = forceField.position - tip;
			float dist;
			if (forceField.type == ENTITY_TYPE_FORCEFIELD_POINT) // point-based force field
			{
				dist = length(dir);
			}
			else // planar force field
			{
				dist = dot(forceField.normal, dir);
				dir = forceField.normal;
			}

			force += dir * forceField.gravity * (1 - saturate(dist * forceField.range_inverse));
		}

		// Pull back to rest position:
		force += (target - normal) * xStiffness;

		// Apply forces:
		velocity += force * g_xFrame_DeltaTime;
		normal += velocity * g_xFrame_DeltaTime;

		// Drag:
		velocity *= 0.98f;

		// Transform back to mesh space and renormalize:
		normal = normalize(mul(normal, (float3x3)xWorld)); // transposed mul!

		// Store particle:
		particleBuffer[particleID].position = position;
		particleBuffer[particleID].normal = normal;

		// Store simulation data:
		simulationBuffer[particleID].velocity = velocity;

		// Offset next segment position to segment root:
		position.xyz += normal * len;
		segmentRootNormal = normal;
	}
}
