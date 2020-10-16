#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Patch, 0);
RWSTRUCTUREDBUFFER(simulationBuffer, PatchSimulationData, 1);
RWSTRUCTUREDBUFFER(indexBuffer, uint, 2);
RWRAWBUFFER(counterBuffer, 3);

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND1);
TYPEDBUFFER(meshVertexBuffer_length, float, TEXSLOT_ONDEMAND2);

#define NUM_LDS_FORCEFIELDS 32
struct LDS_ForceField
{
	uint type;
	float3 position;
	float gravity;
	float range_rcp;
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
		ShaderEntity forceField = EntityArray[forceFieldID];

		forceFields[groupIndex].type = (uint)forceField.GetType();
		forceFields[groupIndex].position = forceField.positionWS;
		forceFields[groupIndex].gravity = forceField.energy;
		forceFields[groupIndex].range_rcp = forceField.range; // it is actually uploaded from CPU as 1.0f / range
		forceFields[groupIndex].normal = forceField.directionWS;
	}
	GroupMemoryBarrierWithGroupSync();

	if (DTid.x >= xHairParticleCount)
		return;

	// Generate patch:

	// random triangle on emitter surface:
	//	(Note that the usual rand() function is not used because that introduces unnatural clustering with high triangle count)
	uint tri = (uint)((xHairBaseMeshIndexCount / 3) * hash1(DTid.x));

	// load indices of triangle from index buffer
	uint i0 = meshIndexBuffer[tri * 3 + 0];
	uint i1 = meshIndexBuffer[tri * 3 + 1];
	uint i2 = meshIndexBuffer[tri * 3 + 2];

	// load vertices of triangle from vertex buffer:
	float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xHairBaseMeshVertexPositionStride));
    float3 nor0 = unpack_unitvector(asuint(pos_nor0.w));
    float3 nor1 = unpack_unitvector(asuint(pos_nor1.w));
    float3 nor2 = unpack_unitvector(asuint(pos_nor2.w));
	float length0 = meshVertexBuffer_length[i0];
	float length1 = meshVertexBuffer_length[i1];
	float length2 = meshVertexBuffer_length[i2];

	// random barycentric coords:
	float2 uv = float2((Gid.x + 1.0f) / (float)xHairNumDispatchGroups, (DTid.x + 1.0f) / (float)THREADCOUNT_SIMULATEHAIR);
	float seed = xHairRandomSeed;
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
	float strand_length = length0 + f * (length1 - length0) + g * (length2 - length0);

	uint tangent_random = 0;
	tangent_random |= (uint)((uint)(tangent.x * 127.5f + 127.5f) << 0);
	tangent_random |= (uint)((uint)(tangent.y * 127.5f + 127.5f) << 8);
	tangent_random |= (uint)((uint)(tangent.z * 127.5f + 127.5f) << 16);
	tangent_random |= (uint)(rand(seed, uv) * 255) << 24;

	uint binormal_length = 0;
	binormal_length |= (uint)((uint)(binormal.x * 127.5f + 127.5f) << 0);
	binormal_length |= (uint)((uint)(binormal.y * 127.5f + 127.5f) << 8);
	binormal_length |= (uint)((uint)(binormal.z * 127.5f + 127.5f) << 16);
	binormal_length |= (uint)(lerp(1, rand(seed, uv), saturate(xHairRandomness)) * strand_length * 255) << 24;

	// Identifies the hair strand root particle:
    const uint strandID = DTid.x * xHairSegmentCount;
    
	// Transform particle by the emitter object matrix:
    float3 base = mul(xWorld, float4(position.xyz, 1)).xyz;
    target = normalize(mul((float3x3)xWorld, target));
	const float3 root = base;

	float3 normal = 0;
    
	for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
	{
		// Identifies the hair strand segment particle:
		const uint particleID = strandID + segmentID;
		particleBuffer[particleID].tangent_random = tangent_random;
		particleBuffer[particleID].binormal_length = binormal_length;

		if (xHairRegenerate)
		{
			particleBuffer[particleID].position = base;
			particleBuffer[particleID].normal = target;
			simulationBuffer[particleID].velocity = 0;
		}

        normal += particleBuffer[particleID].normal;
        normal = normalize(normal);

		float len = (binormal_length >> 24) & 0x000000FF;
		len /= 255.0f;
		len *= xLength;

		float3 tip = base + normal * len;

		// Accumulate forces:
		float3 force = 0;
        for (uint i = 0; i < numForceFields; ++i)
        {
            LDS_ForceField forceField = forceFields[i];

			//float3 dir = forceField.position - PointOnLineSegmentNearestPoint(base, tip, forceField.position);
			float3 dir = forceField.position - tip;
            float dist;
            if (forceField.type == ENTITY_TYPE_FORCEFIELD_POINT) // point-based force field
            {
                //dist = length(dir);
				dist = length(forceField.position - ClosestPointOnSegment(base, tip, forceField.position));
            }
            else // planar force field
            {
                dist = dot(forceField.normal, dir);
                dir = forceField.normal;
            }

            force += dir * forceField.gravity * (1 - saturate(dist * forceField.range_rcp));
        }

		// Pull back to rest position:
        force += (target - normal) * xStiffness;

        force *= g_xFrame_DeltaTime;

		// Simulation buffer load:
        float3 velocity = simulationBuffer[particleID].velocity;

		// Apply surface-movement-based velocity:
		const float3 old_base = particleBuffer[particleID].position;
		const float3 old_normal = particleBuffer[particleID].normal;
		const float3 old_tip = old_base + old_normal * len;
		const float3 surface_velocity = old_tip - tip;
		velocity += surface_velocity;

		// Apply forces:
		velocity += force;
		normal += velocity * g_xFrame_DeltaTime;

		// Drag:
		velocity *= 0.98f;

		// Store particle:
        particleBuffer[particleID].position = base;
		particleBuffer[particleID].normal = normalize(normal);

		// Store simulation data:
		simulationBuffer[particleID].velocity = velocity;

		// Offset next segment root to current tip:
        base = tip;


		// Frustum culling:
		float3 sphereCenter = base + tip * 0.5f;
		float sphereRadius = length(sphereCenter - base);
		bool infrustum = true;
		infrustum = distance(sphereCenter, g_xCamera_CamPos.xyz) > xHairViewDistance ? false : infrustum;
		infrustum = dot(g_xCamera_FrustumPlanes[0], float4(sphereCenter, 1)) < -sphereRadius ? false : infrustum;
		infrustum = dot(g_xCamera_FrustumPlanes[2], float4(sphereCenter, 1)) < -sphereRadius ? false : infrustum;
		infrustum = dot(g_xCamera_FrustumPlanes[3], float4(sphereCenter, 1)) < -sphereRadius ? false : infrustum;
		infrustum = dot(g_xCamera_FrustumPlanes[4], float4(sphereCenter, 1)) < -sphereRadius ? false : infrustum;
		infrustum = dot(g_xCamera_FrustumPlanes[5], float4(sphereCenter, 1)) < -sphereRadius ? false : infrustum;

		if (infrustum)
		{
			uint prevCount;
			counterBuffer.InterlockedAdd(0, 1, prevCount);
			indexBuffer[prevCount] = particleID;
		}
    }
}
