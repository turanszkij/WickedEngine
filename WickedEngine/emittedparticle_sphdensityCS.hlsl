#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
STRUCTUREDBUFFER(counterBuffer, ParticleCounters, 1);

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(densityBuffer, float, 1);

groupshared float3 positions[THREADCOUNT_SIMULATION];

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID )
{
	// SPH params:
	const float h = xSPH_h;			// smoothing radius
	const float h2 = xSPH_h2;		// smoothing radius ^ 2
	const float h3 = xSPH_h3;		// smoothing radius ^ 3
	const float h6 = xSPH_h6;		// smoothing radius ^ 6
	const float h9 = xSPH_h9;		// smoothing radius ^ 9
	const float K = xSPH_K;			// pressure constant
	const float p0 = xSPH_p0;		// reference density
	const float e = xSPH_e;			// viscosity constant
	const float mass = xParticleMass;

	uint aliveCount = counterBuffer[0].aliveCount;

	uint particleIndexA;
	Particle particleA;

	if (DTid.x < aliveCount)
	{
		particleIndexA = aliveBuffer_CURRENT[DTid.x];
		particleA = particleBuffer[particleIndexA];
	}



	// Compute density field:
	float density = 0; // (p)

	uint numTiles = 1 + aliveCount / THREADCOUNT_SIMULATION;

	for (uint tile = 0; tile < numTiles; ++tile)
	{
		uint offset = tile * THREADCOUNT_SIMULATION;
		uint id = offset + groupIndex;

		
		if (id < aliveCount)
		{
			positions[groupIndex] = particleBuffer[aliveBuffer_CURRENT[id]].position;
		}
		else
		{
			positions[groupIndex] = 1000000; // "infinitely far" try to not contribute non existing particles
		}

		GroupMemoryBarrierWithGroupSync();


		for (uint i = 0; i < THREADCOUNT_SIMULATION; ++i)
		{
			if (offset + i != DTid.x)
			{
				float3 positionB = positions[i];

				float3 diff = particleA.position - positionB;
				float r2 = dot(diff, diff); // distance squared

				if (r2 < h2)
				{
					float W = (315.0f / (64.0f * PI * h9)) * pow(h2 - r2, 3); // poly6 smoothing kernel

					//density += particleB.m * W;
					density += mass * W; // constant mass
				}

			}
		}

		GroupMemoryBarrierWithGroupSync();
	}





	if (DTid.x < aliveCount)
	{
		// Can't be lower than reference density to avoid negative pressure!
		density = max(p0, density);

		// Store the results:
		densityBuffer[particleIndexA] = density;

	}


}

