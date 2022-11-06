#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<uint> aliveBuffer_CURRENT : register(t0);
StructuredBuffer<Particle> particleBuffer : register(t1);
StructuredBuffer<uint> particleCellBuffer : register(t2);
StructuredBuffer<SPHGridCell> cellBuffer : register(t3);

RWByteAddressBuffer counterBuffer : register(u0);
RWStructuredBuffer<float> densityBuffer : register(u1);

#ifndef SPH_USE_ACCELERATION_GRID
// grid structure is not a good fit to exploit shared memory because one threadgroup can load from different initial cells :(
groupshared float4 positions_masses[THREADCOUNT_SIMULATION];
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
	float3 positionA;

	if (DTid.x < aliveCount)
	{
		particleIndexA = aliveBuffer_CURRENT[DTid.x];
		positionA = particleBuffer[particleIndexA].position;
	}
	else
	{
		particleIndexA = 0xFFFFFFFF;
		positionA = 0;
	}



	// Compute density field:
	float density = 0; // (p)

#ifdef SPH_USE_ACCELERATION_GRID

	// Grid cell is of size [SPH smoothing radius], so position is refitted into that
	const float3 remappedPos = positionA * xSPH_h_rcp;
	const int3 cellIndex = floor(remappedPos);

	// iterate through all [27] neighbor cells:
	for (uint i = 0; i < 27; ++i)
	{
		// hashed cell index is retrieved:
		const int3 neighborIndex = cellIndex + sph_neighbor_offsets[i];
		const uint flatNeighborIndex = SPH_GridHash(neighborIndex);

		SPHGridCell cell = cellBuffer[flatNeighborIndex];
		for (uint iterator = 0; iterator < cell.count; ++iterator)
		{
			uint particleIndexB = particleCellBuffer[cell.offset + iterator];

			// SPH Density evaluation:
			{
				Particle particleB = particleBuffer[particleIndexB];

				float3 diff = positionA - particleB.position;
				float r2 = dot(diff, diff); // distance squared

				if (r2 < h2)
				{
					float W = xSPH_poly6_constant * pow(h2 - r2, 3); // poly6 smoothing kernel

					density += particleB.mass * W;
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

				float3 diff = positionA - positionB;
				float r2 = dot(diff, diff); // distance squared

				if (r2 < h2)
				{
					float W = xSPH_poly6_constant * pow(h2 - r2, 3); // poly6 smoothing kernel

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

