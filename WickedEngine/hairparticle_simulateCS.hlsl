#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Patch, 0);
RWSTRUCTUREDBUFFER(simulationBuffer, PatchSimulationData, 1);

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
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	// Load the forcefields into LDS:
	uint numForceFields = min(g_xFrame_ForceFieldArrayCount, NUM_LDS_FORCEFIELDS);
	if (Gid < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldArrayOffset + Gid;
		ShaderEntityType forceField = EntityArray[forceFieldID];

		forceFields[Gid].type = (uint)forceField.type;
		forceFields[Gid].position = forceField.positionWS;
		forceFields[Gid].gravity = forceField.energy;
		forceFields[Gid].range_inverse = forceField.range;
		forceFields[Gid].normal = forceField.directionWS;
	}

	GroupMemoryBarrierWithGroupSync();


	const uint instanceID = DTid.x;

	// Particle buffer load:
	float3 position = particleBuffer[instanceID].position;
	float3 normal = particleBuffer[instanceID].normal;

	// Simulation buffer load:
	float3 velocity = simulationBuffer[instanceID].velocity;
	uint uTarget = simulationBuffer[instanceID].target;
	float3 target;
	target.x = (uTarget >> 0) & 0x000000FF;
	target.y = (uTarget >> 8) & 0x000000FF;
	target.z = (uTarget >> 16) & 0x000000FF;
	target = target / 255.0f * 2 - 1;

	// transform particle by the emitter object matrix:
	position.xyz = mul(xWorld, float4(position.xyz, 1)).xyz;
	normal = normalize(mul((float3x3)xWorld, normal));
	target = normalize(mul((float3x3)xWorld, target));

	// Accumulate forces:
	float3 force = 0;
	for (uint i = 0; i < numForceFields; ++i)
	{
		LDS_ForceField forceField = forceFields[i];

		float3 dir = forceField.position - position;
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

	// Store particle normal:
	particleBuffer[instanceID].normal = normal;

	// Store simulation data:
	simulationBuffer[instanceID].velocity = velocity;
}
