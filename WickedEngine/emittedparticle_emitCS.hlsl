#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_OLD, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, uint3, 4);

TEXTURE2D(randomTex, float4, TEXSLOT_ONDEMAND0);


[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount_OLD = counterBuffer[0][0];
	uint deadCount = counterBuffer[0][1];
	uint emitCount = xEmitCount;



	Particle particle;
	particle.position = 0;
	particle.size = 1;
	particle.rotation = 0;
	particle.velocity = float3(0, 0.1, 0);
	particle.rotationalVelocity = 0;
	particle.maxLife = 1;
	particle.life = particle.maxLife;
	particle.opacity = 1;
	particle.sizeBeginEnd = float2(particle.size, particle.size);
	particle.mirror = float2(0, 0);

	particleBuffer[0] = particle;


	while(deadCount > 0 && emitCount > 0)
	{
		// we can emit

		uint newParticleIndex = deadBuffer[deadCount - 1];

		particleBuffer[newParticleIndex] = particle;

		aliveBuffer_OLD[aliveCount_OLD] = newParticleIndex;

		aliveCount_OLD++;
		deadCount--;
		emitCount--;
	}

	counterBuffer[0] = uint3(aliveCount_OLD, deadCount, 0);
}
