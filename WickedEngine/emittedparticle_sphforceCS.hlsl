#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

// enable pressure visualizer debug colors:
//  green - under reference pressure
//  red - above reference pressure
#define DEBUG_PRESSURE

#define FLOOR_COLLISION
#define BOX_COLLISION

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
	const float mass = xParticleMass;	// constant mass (todo: per particle)

	uint aliveCount = counterBuffer[0].aliveCount;


	if (DTid.x < aliveCount)
	{
		uint particleIndexA = aliveBuffer_CURRENT[DTid.x];
		Particle particleA = particleBuffer[particleIndexA];

		float densityA = densityBuffer[particleIndexA].x;
		float pressureA = densityBuffer[particleIndexA].y;

		// Compute acceleration:
		float3 f_a = 0;	// pressure force
		float3 f_av = 0;  // viscosity force

		for (uint i = 0; i < aliveCount; ++i)
		{
			if (i != DTid.x)
			{
				uint particleIndexB = aliveBuffer_CURRENT[i];
				Particle particleB = particleBuffer[particleIndexB];

				float3 diff = particleA.position - particleB.position;
				float r2 = dot(diff, diff); // distance squared
				float r = sqrt(r2);

				if (r < h)
				{
					float densityB = densityBuffer[particleIndexB].x;
					float pressureB = densityBuffer[particleIndexB].y;

					float3 rNorm = normalize(diff);
					float W = (-45 / (PI * h6)) * pow(h - r, 2); // spiky kernel smoothing function

					//float mass = particleB.mass / particleA.mass;

					f_a += mass * ((pressureA + pressureB) / (2 * densityA * densityB)) * W * rNorm;

					float r3 = r2 * r;
					W = -(r3 / (2 * h3)) + (r2 / h2) + (h / (2 * r)) - 1; // laplacian smoothing function
					f_av += mass * (1.0f / densityB) * (particleB.velocity - particleA.velocity) * W * rNorm;
				}

			}
		}

		f_a *= -1;
		f_av *= e;

		// gravity:
		const float3 G = float3(0, -9.8f, 0);

		// apply all forces:
		float3 force = (f_a + f_av) / densityA + G;

		// integrate:
		const float dt = g_xFrame_DeltaTime;
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
		float3 extent = float3(10, 0, 8);
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

