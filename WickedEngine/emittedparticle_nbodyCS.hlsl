#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);

struct LDSParticle
{
	float3 position;
	float size;
	float3 v;			// velocity
	float m;			// mass
	float p;			// density
	float P;			// Pressure
};
groupshared LDSParticle LDSParticles[THREADCOUNT_SIMULATION];

[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID )
{
	uint aliveCount = counterBuffer[0].aliveCount;

	if (DTid.x < aliveCount)
	{
		uint particleIndexA = aliveBuffer_CURRENT[DTid.x];
		Particle particleA = particleBuffer[particleIndexA];

		float lifeLerpA = 1 - particleA.life / particleA.maxLife;
		float particleSizeA = lerp(particleA.sizeBeginEnd.x, particleA.sizeBeginEnd.y, lifeLerpA);

		LDSParticles[groupIndex].position = particleA.position;
		LDSParticles[groupIndex].size = particleSizeA;
		LDSParticles[groupIndex].v = particleA.velocity;
		LDSParticles[groupIndex].m = xParticleMass;
		LDSParticles[groupIndex].p = 0;
		LDSParticles[groupIndex].P = 0;

	}

	GroupMemoryBarrierWithGroupSync();

	const uint LDSParticleCount = /*clamp(aliveCount - Gid.x * THREADCOUNT_SIMULATION, 0, THREADCOUNT_SIMULATION)*/ 256;


	uint particleIndexA = groupIndex;
	LDSParticle particleA = LDSParticles[particleIndexA];


	// SPH params:
	const float h = xSPH_h;			// smoothing radius
	const float h2 = xSPH_h2;		// smoothing radius ^ 2
	const float h3 = xSPH_h3;		// smoothing radius ^ 3
	const float h6 = xSPH_h6;		// smoothing radius ^ 6
	const float h9 = xSPH_h9;		// smoothing radius ^ 9
	const float K = xSPH_K;			// pressure constant
	const float p0 = xSPH_p0;		// reference density
	const float e = xSPH_e;			// viscosity constant


	// Compute density field:

	uint i;

	for (i = 0; i < LDSParticleCount; ++i)
	{
		if (i != particleIndexA)
		{
			uint particleIndexB = i;
			LDSParticle particleB = LDSParticles[particleIndexB];

			float3 diff = particleA.position - particleB.position;
			float r2 = dot(diff, diff); // distance squared

			//float range = particleA.size + particleB.size; // range of affection
			//float range2 = range * range; // range squared

			if (r2 < h2)
			{
				float W = (315.0f / (64.0f * PI * h9)) * pow(h2 - r2, 3); // poly6 smoothing kernel

				particleA.p += particleB.m * W;
			}

		}
	}

	int asd = 0;
	if (particleA.p > p0) asd = 1;
	if (particleA.p < p0) asd = -1;

	// Can't be lower than reference density to avoid negative pressure!
	//particleA.p = max(p0, particleA.p);

	// Compute particle pressure:
	particleA.P = K * (max(p0, particleA.p) - p0);

	// Store the results:
	LDSParticles[particleIndexA].p = particleA.p;
	LDSParticles[particleIndexA].P = particleA.P;


	// Wait for all particles to compute pressure
	GroupMemoryBarrierWithGroupSync();


	if (particleA.p > 0)
	{

		// Compute acceleration:
		float3 a = 0;	// pressure force
		float3 av = 0;  // viscosity force

		for (i = 0; i < LDSParticleCount; ++i)
		{
			if (i != particleIndexA)
			{
				uint particleIndexB = i;
				LDSParticle particleB = LDSParticles[particleIndexB];

				float3 diff = particleA.position - particleB.position;
				float r2 = dot(diff, diff); // distance squared
				float r = sqrt(r2);

				//float range = particleA.size + particleB.size; // range of affection

				if (r < h)
				{
					float3 rNorm = normalize(diff);
					float W = (-45 / (PI * h6)) * pow(h - r, 2); // spiky kernel smoothing function

					a += (particleB.m / particleA.m) * ((particleA.P + particleB.P) / (2 * particleA.p * particleB.p)) * W * rNorm;

					float r3 = r2 * r;
					W = -(r3 / (2 * h3)) + (r2 / h2) + (h / (2 * r)) - 1;
					av += (particleB.m / particleA.m) * (1.0f / particleB.p) * (particleB.v - particleA.v) * W * rNorm;
				}

			}
		}

		a *= -1;
		av *= e;

		float3 force = a + av;

		const float dt = g_xFrame_DeltaTime;
		particleA.v += dt * force / particleA.p;
		particleA.position += dt * particleA.v;

	}

	float elastic = 0.9;

	if (particleA.position.y - particleA.size < 0)
	{
		particleA.position.y = particleA.size;
		particleA.v.y *= -elastic;
	}

	// box collision:
	float extent = 2;
	if (particleA.position.x + particleA.size > extent)
	{
		particleA.position.x = extent - particleA.size;
		particleA.v.x *= -elastic;
	}
	if (particleA.position.x - particleA.size < -extent)
	{
		particleA.position.x = -extent + particleA.size;
		particleA.v.x *= -elastic;
	}
	if (particleA.position.z + particleA.size > extent)
	{
		particleA.position.z = extent - particleA.size;
		particleA.v.z *= -elastic;
	}
	if (particleA.position.z - particleA.size < -extent)
	{
		particleA.position.z = -extent + particleA.size;
		particleA.v.z *= -elastic;
	}

	particleA.v *= 0.99f;

	particleA.v.y -= 0.8f;


	if (DTid.x < aliveCount)
	{
		uint writeIndex = aliveBuffer_CURRENT[DTid.x];
		particleBuffer[writeIndex].position = particleA.position;
		particleBuffer[writeIndex].velocity = particleA.v;

		particleBuffer[writeIndex].color_mirror = 0x00FFFFFF;
		//particleBuffer[writeIndex].color_mirror |= ((uint)particleA.p) & 0xFF;

		if (asd > 0)
		{
			particleBuffer[writeIndex].color_mirror = 0xFF;
		}
		if (asd < 0)
		{
			particleBuffer[writeIndex].color_mirror = 0xFF00;
		}

	}


}

