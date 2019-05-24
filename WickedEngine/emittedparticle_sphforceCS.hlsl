#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

//#define DEBUG_PRESSURE

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(densityBuffer, float, 2);
STRUCTUREDBUFFER(cellIndexBuffer, float, 3);
STRUCTUREDBUFFER(cellOffsetBuffer, uint, 4);

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);

#ifndef SPH_USE_ACCELERATION_GRID
// grid structure is not a good fit to exploit shared memory because one threadgroup can load from different initial cells :(
groupshared float4 positions_densities[THREADCOUNT_SIMULATION];
groupshared float4 velocities_pressures[THREADCOUNT_SIMULATION];
groupshared float masses[THREADCOUNT_SIMULATION];
#endif // SPH_USE_ACCELERATION_GRID

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID )
{
	// SPH params:
	const float h = xSPH_h;			// smoothing radius
	const float h2 = xSPH_h2;		// smoothing radius ^ 2
	const float h3 = xSPH_h3;		// smoothing radius ^ 3
	const float K = xSPH_K;			// pressure constant
	const float p0 = xSPH_p0;		// reference density
	const float e = xSPH_e;			// viscosity constant

	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	uint particleIndexA;
	Particle particleA;
	float densityA;
	float pressureA;

	if (DTid.x < aliveCount)
	{
		particleIndexA = aliveBuffer_CURRENT[DTid.x];
		particleA = particleBuffer[particleIndexA];
		densityA = densityBuffer[particleIndexA];
		pressureA = K * (densityA - p0);
	}
	else
	{
		particleIndexA = 0xFFFFFFFF;
		particleA = (Particle)0;
		densityA = 0;
		pressureA = 0;
	}



	// Compute acceleration:
	float3 f_a = 0;	// pressure force
	float3 f_av = 0;  // viscosity force


#ifdef SPH_USE_ACCELERATION_GRID

	// Grid cell is of size [SPH smoothing radius], so position is refitted into that
	const float3 remappedPos = particleA.position  * xSPH_h_rcp;
	const int3 cellIndex = floor(remappedPos);

	// iterate through all [27] neighbor cells:
	[loop]
	for (int i = -1; i <= 1; ++i)
	{
		[loop]
		for (int j = -1; j <= 1; ++j)
		{
			[loop]
			for (int k = -1; k <= 1; ++k)
			{
				// hashed cell index is retrieved:
				const int3 neighborIndex = cellIndex + int3(i, j, k);
				const uint flatNeighborIndex = SPH_GridHash(neighborIndex);

				// look up the offset into particle list from neighbor cell:
				uint neighborIterator = cellOffsetBuffer[flatNeighborIndex];

				// iterate through neighbor cell particles (if iterator offset is valid):
				[loop]
				while (neighborIterator != 0xFFFFFFFF && neighborIterator < aliveCount)
				{
					if (neighborIterator != DTid.x)
					{
						uint particleIndexB = aliveBuffer_CURRENT[neighborIterator];
						if ((uint)cellIndexBuffer[particleIndexB] != flatNeighborIndex)
						{
							// here means we stepped out of the neighbor cell list!
							break;
						}

						// SPH Force evaluation:
						{
							const float3 positionB = particleBuffer[particleIndexB].position.xyz;

							const float3 diff = particleA.position - positionB;
							const float r2 = dot(diff, diff); // distance squared
							const float r = sqrt(r2);

							if (r > 0 && r < h) // avoid division by zero!
							{
								const float3 velocityB = particleBuffer[particleIndexB].velocity.xyz;
								const float densityB = densityBuffer[particleIndexB];
								const float pressureB = K * (densityB - p0);

								const float3 rNorm = diff / r;
								float W = xSPH_spiky_constant * pow(h - r, 2); // spiky kernel smoothing function

								const float mass = particleBuffer[particleIndexB].mass / particleA.mass;

								f_a += mass * ((pressureA + pressureB) / (2 * densityA * densityB)) * W * rNorm;

								float r3 = r2 * r;
								W = -(r3 / (2 * h3)) + (r2 / h2) + (h / (2 * r)) - 1; // laplacian smoothing function
								f_av += mass * (1.0f / densityB) * (velocityB - particleA.velocity) * W * rNorm;
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
			uint particleIndex = aliveBuffer_CURRENT[id];

			float density = densityBuffer[particleIndex];
			positions_densities[groupIndex] = float4(particleBuffer[particleIndex].position, density);

			float pressure = K * (density - p0);
			velocities_pressures[groupIndex] = float4(particleBuffer[particleIndex].velocity, pressure);

			masses[groupIndex] = particleBuffer[particleIndex].mass;
		}
		else
		{
			positions_densities[groupIndex] = float4(1000000, 1000000, 1000000, 0); // "infinitely far" try to not contribute non existing particles, zero density
			velocities_pressures[groupIndex] = float4(0, 0, 0, 0);
			masses[groupIndex] = 0;
		}

		GroupMemoryBarrierWithGroupSync();


		for (uint i = 0; i < THREADCOUNT_SIMULATION; ++i)
		{
			if (offset + i != DTid.x)
			{
				float3 positionB = positions_densities[i].xyz;

				float3 diff = particleA.position - positionB;
				float r2 = dot(diff, diff); // distance squared
				float r = sqrt(r2);

				if (r > 0 && r < h) // avoid division by zero!
				{
					float3 velocityB = velocities_pressures[i].xyz;
					float densityB = positions_densities[i].w;
					float pressureB = velocities_pressures[i].w;

					float3 rNorm = diff / r;
					float W = xSPH_spiky_constant * pow(h - r, 2); // spiky kernel smoothing function

					float mass = masses[i] / particleA.mass;

					f_a += mass * ((pressureA + pressureB) / (2 * densityA * densityB)) * W * rNorm;

					float r3 = r2 * r;
					W = -(r3 / (2 * h3)) + (r2 / h2) + (h / (2 * r)) - 1; // laplacian smoothing function
					f_av += mass * (1.0f / densityB) * (velocityB - particleA.velocity) * W * rNorm;
				}

			}
		}


		GroupMemoryBarrierWithGroupSync();
	}

#endif // SPH_USE_ACCELERATION_GRID


	if (DTid.x < aliveCount)
	{
		// optimize formulae:
		f_a *= -1;
		f_av *= e;

		// gravity:
		const float3 G = float3(0, -9.8f * 2, 0);

		// apply all forces:
		particleA.force += (f_a + f_av) / densityA + G;

		particleBuffer[particleIndexA].force = particleA.force;


#ifdef DEBUG_PRESSURE
		// debug pressure:
		float3 color = lerp(float3(1, 1, 1), float3(1, 0, 0), pow(saturate(pressureA * 0.0005f), 0.5)) * 255;
		uint uColor = 0;
		uColor |= (uint)color.r << 0;
		uColor |= (uint)color.g << 8;
		uColor |= (uint)color.b << 16;
		particleBuffer[particleIndexA].color_mirror = uColor;
#endif // DEBUG_PRESSURE

	}


}

