#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_OLD, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, uint4, 4);

TEXTURE2D(randomTex, float4, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshIndexBuffer, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
RAWBUFFER(meshVertexBuffer_NOR, TEXSLOT_ONDEMAND3);


[numthreads(THREADCOUNT_EMIT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint emitCount = counterBuffer[0][3];

	if(DTid.x < emitCount)
	{
		// we can emit:

		const float random = randomTex.SampleLevel(sampler_linear_wrap, float2(sin(g_xFrame_Time*DTid.x*xRandomness), cos(g_xFrame_Time*DTid.x*xRandomness)) * 0.5f + 0.5f, 0).r;
		uint meshIndex = meshIndexBuffer.Load(floor(xMeshIndexCount * random) * xMeshIndexStride);
		meshIndex = (xMeshIndexStride == 2 ? (meshIndex & 0x0000FFFF) : meshIndex);
		float3 randomPos = asfloat(meshVertexBuffer_POS.Load3(meshIndex * xMeshVertexPositionStride));

		uint nor_u = meshVertexBuffer_NOR.Load(meshIndex * xMeshVertexNormalStride);
		float3 randomNormal;
		{
			randomNormal.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			randomNormal.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			randomNormal.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}

		Particle particle;
		particle.position = mul(xEmitterWorld, float4(randomPos, 1)).xyz;
		particle.size = 0.2;
		particle.rotation = 0;
		particle.velocity = randomNormal * 0.01f;
		particle.rotationalVelocity = 0;
		particle.maxLife = 60*4;
		particle.life = particle.maxLife;
		particle.opacity = 1;
		particle.sizeBeginEnd = float2(particle.size, particle.size);
		particle.mirror = float2(0, 0);

		uint deadCount;
		InterlockedAdd(counterBuffer[0][1], -1, deadCount);

		uint newParticleIndex = deadBuffer[deadCount - 1];

		particleBuffer[newParticleIndex] = particle;


		uint aliveCount_OLD;
		InterlockedAdd(counterBuffer[0][0], 1, aliveCount_OLD);
		aliveBuffer_OLD[aliveCount_OLD] = newParticleIndex;
	}
}
