#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);
RWRAWBUFFER(indirectBuffers, 5);
RWSTRUCTUREDBUFFER(distanceBuffer, float, 6);

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
groupshared uint aliveParticleCount = 0;


[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	// Read alive particle count and store in LDS:
	if (Gid == 0)
	{
		aliveParticleCount = counterBuffer[0].aliveCount;
	}

	// Load the forcefields into LDS:
	uint numForceFields = min(g_xFrame_ForceFieldCount, NUM_LDS_FORCEFIELDS);
	if (Gid < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldOffset + Gid;
		ShaderEntityType forceField = EntityArray[forceFieldID];

		forceFields[Gid].type = (uint)forceField.type;
		forceFields[Gid].position = forceField.positionWS;
		forceFields[Gid].gravity = forceField.energy;
		forceFields[Gid].range_inverse = forceField.range;
		forceFields[Gid].normal = forceField.directionWS;
	}

	GroupMemoryBarrierWithGroupSync();

	if (DTid.x < aliveParticleCount)
	{
		const float dt = g_xFrame_DeltaTime;

		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		if (particle.life > 0)
		{
			// simulate:

			for (uint i = 0; i < numForceFields; ++i)
			{
				LDS_ForceField forceField = forceFields[i];

				float3 dir = forceField.position - particle.position;
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

				float3 force = dir * forceField.gravity * (1 - saturate(dist * forceField.range_inverse));

				particle.velocity += force * dt;
			}

			particle.position += particle.velocity * dt;
			particle.rotation += particle.rotationalVelocity * dt;

			float lifeLerp = 1 - particle.life / particle.maxLife;
			particle.size = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);
			particle.opacity = lerp(1, 0, lifeLerp);

			particle.life -= dt;

			// write back simulated particle:
			particleBuffer[particleIndex] = particle;

			// add to new alive list:
			uint newAliveIndex;
			indirectBuffers.InterlockedAdd(24, 6, newAliveIndex); // write the draw argument buffer, which should contain particle count * 6
			aliveBuffer_NEW[newAliveIndex / 6] = particleIndex; // draw arg buffer contains particle count * 6, so just divide to retrieve correct index

			if (xEmitterSortingEnabled)
			{
				// store squared distance to main camera:
				float3 eyeVector = particle.position - g_xFrame_MainCamera_CamPos;
				float distSQ = dot(eyeVector, eyeVector);
				distanceBuffer[newAliveIndex / 6] = distSQ;
			}
		}
		else
		{
			// kill:
			uint deadIndex;
			InterlockedAdd(counterBuffer[0].deadCount, 1, deadIndex);
			deadBuffer[deadIndex] = particleIndex;
		}
	}
}
