#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_OLD, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, uint4, 4);

#define NUM_LDS_FORCEFIELDS 32
struct LDS_ForceField
{
	uint type;
	float3 position;
	float gravity;
	float range;
};
groupshared LDS_ForceField forceFields[NUM_LDS_FORCEFIELDS];


[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	// First, load the forcefields to LDS:
	uint numForceFields = min(g_xFrame_ForceFieldCount, NUM_LDS_FORCEFIELDS);
	if (Gid < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldOffset + Gid;
		ShaderEntityType forceField = EntityArray[forceFieldID];

		forceFields[Gid].type = (uint)forceField.type;
		forceFields[Gid].position = forceField.positionWS;
		forceFields[Gid].gravity = forceField.energy;
		forceFields[Gid].range = max(0.001f, forceField.range);
	}

	GroupMemoryBarrierWithGroupSync();


	uint aliveCount = counterBuffer[0][0];

	if (DTid.x < aliveCount)
	{
		uint dt = g_xFrame_DeltaTime * 60.0f;

		uint particleIndex = aliveBuffer_OLD[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		if (particle.life > 0)
		{
			// simulate:

			for (uint i = 0; i < numForceFields; ++i)
			{
				LDS_ForceField forceField = forceFields[i];

				float3 diff = forceField.position - particle.position;
				float dist = length(diff);
				float3 force = diff * forceField.gravity * (1 - saturate(dist / forceField.range));

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
			InterlockedAdd(counterBuffer[0][2], 1, newAliveIndex);
			aliveBuffer_NEW[newAliveIndex] = particleIndex;
		}
		else
		{
			// kill:
			uint deadIndex;
			InterlockedAdd(counterBuffer[0][1], 1, deadIndex);
			deadBuffer[deadIndex] = particleIndex;
		}
	}
}
