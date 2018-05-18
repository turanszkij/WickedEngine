#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

//#define DEBUG_PRESSURE

#define FLOOR_COLLISION
#define BOX_COLLISION

STRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 0);
RAWBUFFER(counterBuffer, 1);
STRUCTUREDBUFFER(densityBuffer, float, 2);
STRUCTUREDBUFFER(cellIndexBuffer, float, 3);
STRUCTUREDBUFFER(cellOffsetBuffer, uint, 4);

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);

groupshared float4 positions_densities[THREADCOUNT_SIMULATION];
groupshared float4 velocities_pressures[THREADCOUNT_SIMULATION];

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
	const float mass = xParticleMass;	// constant mass (todo: per particle)

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



	// Compute acceleration:
	float3 f_a = 0;	// pressure force
	float3 f_av = 0;  // viscosity force


#ifdef SPH_USE_ACCELERATION_GRID

	// Grid cell is of size [SPH smoothing radius], so position is refitted into that
	float3 remappedPos = particleA.position / xSPH_h; // optimize div??

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
					if (neighborIterator != DTid.x)
					{
						uint particleIndexB = aliveBuffer_CURRENT[neighborIterator];
						if ((uint)cellIndexBuffer[particleIndexB] != flatNeighborIndex)
						{
							break;
						}

						// SPH Force evaluation:
						{
							float3 positionB = particleBuffer[particleIndexB].position.xyz;

							float3 diff = particleA.position - positionB;
							float r2 = dot(diff, diff); // distance squared
							float r = sqrt(r2);

							if (r < h)
							{
								float3 velocityB = particleBuffer[particleIndexB].velocity.xyz;
								float densityB = densityBuffer[particleIndexB];
								float pressureB = K * (densityB - p0);

								float3 rNorm = normalize(diff);
								float W = (-45 / (PI * h6)) * pow(h - r, 2); // spiky kernel smoothing function

								//float mass = particleB.mass / particleA.mass;

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
		}
		else
		{
			positions_densities[groupIndex] = float4(1000000, 1000000, 1000000, 0); // "infinitely far" try to not contribute non existing particles, zero density
			velocities_pressures[groupIndex] = float4(0, 0, 0, 0);
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

				if (r < h)
				{
					float3 velocityB = velocities_pressures[i].xyz;
					float densityB = positions_densities[i].w;
					float pressureB = velocities_pressures[i].w;

					float3 rNorm = normalize(diff);
					float W = (-45 / (PI * h6)) * pow(h - r, 2); // spiky kernel smoothing function

					//float mass = particleB.mass / particleA.mass;

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
		const float3 G = float3(0, -9.8f, 0);

		// apply all forces:
		float3 force = (f_a + f_av) / densityA + G;

		// integrate:
		//const float dt = g_xFrame_DeltaTime; // variable timestep
		const float dt = 0.016f; // fixed time step, otherwise simulation will just blow up
		particleA.velocity += dt * force;
		particleA.position += dt * particleA.velocity;

		// drag:
		particleA.velocity *= 0.98f;





		float lifeLerp = 1 - particleA.life / particleA.maxLife;
		float particleSize = lerp(particleA.sizeBeginEnd.x, particleA.sizeBeginEnd.y, lifeLerp);


		float elastic = 0.6;

#ifdef FLOOR_COLLISION
		// floor collision:
		if (particleA.position.y - particleSize < 0)
		{
			particleA.position.y = particleSize;
			particleA.velocity.y *= -elastic;
		}
#endif // FLOOR_COLLISION


#ifdef BOX_COLLISION
		// box collision:
		float3 extent = float3(20, 0, 12);
		if (particleA.position.x + particleSize > extent.x)
		{
			particleA.position.x = extent.x - particleSize;
			particleA.velocity.x *= -elastic;
		}
		if (particleA.position.x - particleSize < -extent.x)
		{
			particleA.position.x = -extent.x + particleSize;
			particleA.velocity.x *= -elastic;
		}
		if (particleA.position.z + particleSize > extent.z)
		{
			particleA.position.z = extent.z - particleSize;
			particleA.velocity.z *= -elastic;
		}
		if (particleA.position.z - particleSize < -extent.z)
		{
			particleA.position.z = -extent.z + particleSize;
			particleA.velocity.z *= -elastic;
		}
#endif // BOX_COLLISION

		particleBuffer[particleIndexA].position = particleA.position;
		particleBuffer[particleIndexA].velocity = particleA.velocity;


#ifdef DEBUG_PRESSURE
		// debug pressure:
		float3 color = lerp(float3(1, 1, 1), float3(1, 0, 0), saturate(abs(pressureA - p0) * 0.01f)) * 255;
		uint uColor = 0;
		uColor |= (uint)color.r << 0;
		uColor |= (uint)color.g << 8;
		uColor |= (uint)color.b << 16;
		particleBuffer[particleIndexA].color_mirror = uColor;
#endif // DEBUG_PRESSURE

	}


}

