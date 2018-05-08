#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);

RWSTRUCTUREDBUFFER(densityBuffer, float2, 7);

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


	if (DTid.x < aliveCount)
	{
		uint particleIndexA = aliveBuffer_CURRENT[DTid.x];
		Particle particleA = particleBuffer[particleIndexA];

		// Compute density field:
		float density = 0; // (p)
		for (uint i = 0; i < aliveCount; ++i)
		{
			if (i != DTid.x)
			{
				uint particleIndexB = aliveBuffer_CURRENT[i];
				Particle particleB = particleBuffer[particleIndexB];

				float3 diff = particleA.position - particleB.position;
				float r2 = dot(diff, diff); // distance squared

				if (r2 < h2)
				{
					float W = (315.0f / (64.0f * PI * h9)) * pow(h2 - r2, 3); // poly6 smoothing kernel

					//density += particleB.m * W;
					density += mass * W; // constant mass
				}

			}
		}

		// Can't be lower than reference density to avoid negative pressure!
		density = max(p0, density);

		// Compute particle pressure (P):
		float pressure = K * (density - p0);

		// Store the results:
		densityBuffer[particleIndexA] = float2(density, pressure);

	}


}

