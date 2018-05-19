#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(particleBuffer, Particle, 2);
STRUCTUREDBUFFER(cellIndexBuffer, float, 3);
STRUCTUREDBUFFER(cellOffsetBuffer, uint, 4);

RWSTRUCTUREDBUFFER(densityBuffer, float, 0);

groupshared float4 positions_masses[THREADCOUNT_SIMULATION];

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

	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	uint particleIndexA;
	Particle particleA;

	if (DTid.x < aliveCount)
	{
		particleIndexA = aliveBuffer_CURRENT[DTid.x];
		particleA = particleBuffer[particleIndexA];
	}
	else
	{
		particleIndexA = 0xFFFFFFFF;
		particleA = (Particle)0;
	}



	// Compute density field:
	float density = 0; // (p)

#ifdef SPH_USE_ACCELERATION_GRID

	// Grid cell is of size [SPH smoothing radius], so position is refitted into that
	float3 remappedPos = particleA.position * xSPH_h_rcp;

	int3 cellIndex = floor(remappedPos);

	[loop]
	for (int i = -1; i <= 1; ++i)
	{
		[loop]
		for (int j = -1; j <= 1; ++j)
		{
			[loop]
			for (int k = -1; k <= 1; ++k)
			{
				const int3 neighborIndex = cellIndex + int3(i, j, k);

				const uint flatNeighborIndex = SPH_GridHash(neighborIndex);

				uint neighborIterator = cellOffsetBuffer[flatNeighborIndex];

				[loop]
				while (neighborIterator != 0xFFFFFFFF && neighborIterator < aliveCount)
				{
					//if (neighborIterator != DTid.x) // actually, without this check, the whole thing is just more stable
					{
						uint particleIndexB = aliveBuffer_CURRENT[neighborIterator];
						if ((uint)cellIndexBuffer[particleIndexB] != flatNeighborIndex)
						{
							break;
						}

						// SPH Density evaluation:
						{
							Particle particleB = particleBuffer[particleIndexB];

							float3 diff = particleA.position - particleB.position;
							float r2 = dot(diff, diff); // distance squared

							if (r2 < h2)
							{
								float W = (315.0f / (64.0f * PI * h9)) * pow(h2 - r2, 3); // poly6 smoothing kernel

								density += particleB.mass * W;
							}
						}
					}


					neighborIterator++;
				}
			}
		}
	}

#else

	uint numTiles = 1 + aliveCount / THREADCOUNT_SIMULATION;

	for (uint tile = 0; tile < numTiles; ++tile)
	{
		uint offset = tile * THREADCOUNT_SIMULATION;
		uint id = offset + groupIndex;

		
		if (id < aliveCount)
		{
			Particle particleB = particleBuffer[aliveBuffer_CURRENT[id]];
			positions_masses[groupIndex] = float4(particleB.position, particleB.mass);
		}
		else
		{
			positions_masses[groupIndex] = float4(1000000, 1000000, 1000000, 0); // "infinitely far" try to not contribute non existing particles, zero mass
		}

		GroupMemoryBarrierWithGroupSync();


		for (uint i = 0; i < THREADCOUNT_SIMULATION; ++i)
		{
			//if (offset + i != DTid.x) // actually, without this check, the whole thing is just more stable
			{
				float3 positionB = positions_masses[i].xyz;

				float3 diff = particleA.position - positionB;
				float r2 = dot(diff, diff); // distance squared

				if (r2 < h2)
				{
					float W = (315.0f / (64.0f * PI * h9)) * pow(h2 - r2, 3); // poly6 smoothing kernel

					float mass = positions_masses[i].w;

					density += mass * W;
				}

			}
		}

		GroupMemoryBarrierWithGroupSync();
	}

#endif // SPH_USE_ACCELERATION_GRID




	if (DTid.x < aliveCount)
	{
		// Can't be lower than reference density to avoid negative pressure!
		density = max(p0, density);

		// Store the results:
		densityBuffer[particleIndexA] = density;

	}


}

