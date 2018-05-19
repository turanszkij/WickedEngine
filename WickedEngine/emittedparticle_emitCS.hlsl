#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWRAWBUFFER(counterBuffer, 4);

TEXTURE2D(randomTex, float4, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);


[numthreads(THREADCOUNT_EMIT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint emitCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_REALEMITCOUNT);

	if(DTid.x < emitCount)
	{
		// we can emit:

		const float3 randoms = randomTex.SampleLevel(sampler_linear_wrap, float2((float)DTid.x / (float)THREADCOUNT_EMIT, g_xFrame_Time + xEmitterRandomness), 0).rgb;
		
		// random triangle on emitter surface:
		uint tri = (uint)(((xEmitterMeshIndexCount - 1) / 3) * randoms.x);

		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 1];
		uint i2 = meshIndexBuffer[tri * 3 + 2];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xEmitterMeshVertexPositionStride));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xEmitterMeshVertexPositionStride));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xEmitterMeshVertexPositionStride));

		uint nor_u = asuint(pos_nor0.w);
		float3 nor0;
		{
			nor0.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = asuint(pos_nor1.w);
		float3 nor1;
		{
			nor1.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = asuint(pos_nor2.w);
		float3 nor2;
		{
			nor2.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}

		// random barycentric coords:
		float f = randoms.x;
		float g = randoms.y;
		[flatten]
		if (f + g > 1)
		{
			f = 1 - f;
			g = 1 - g;
		}

		// compute final surface position on triangle from barycentric coords:
		float3 pos = pos_nor0.xyz + f * (pos_nor1.xyz - pos_nor0.xyz) + g * (pos_nor2.xyz - pos_nor0.xyz);
		float3 nor = nor0 + f * (nor1 - nor0) + g * (nor2 - nor0);
		pos = mul(xEmitterWorld, float4(pos, 1)).xyz;
		nor = normalize(mul((float3x3)xEmitterWorld, nor));

		float particleStartingSize = xParticleSize + xParticleSize * (randoms.y - 0.5f) * xParticleRandomFactor;

		// create new particle:
		Particle particle;
		particle.position = pos;
		particle.force = 0;
		particle.mass = xParticleMass;
		particle.velocity = (nor + (randoms.xyz - 0.5f) * xParticleRandomFactor) * xParticleNormalFactor;
		particle.rotationalVelocity = xParticleRotation * (randoms.z - 0.5f) * xParticleRandomFactor;
		particle.maxLife = xParticleLifeSpan + xParticleLifeSpan * (randoms.x - 0.5f) * xParticleLifeSpanRandomness;
		particle.life = particle.maxLife;
		particle.sizeBeginEnd = float2(particleStartingSize, particleStartingSize * xParticleScaling);
		particle.color_mirror = 0;
		particle.color_mirror |= ((randoms.x > 0.5f) << 31) & 0x10000000;
		particle.color_mirror |= ((randoms.y < 0.5f) << 30) & 0x20000000;
		particle.color_mirror |= xParticleColor & 0x00FFFFFF;


		// new particle index retrieved from dead list (pop):
		uint deadCount;
		counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_DEADCOUNT, -1, deadCount);
		uint newParticleIndex = deadBuffer[deadCount - 1];

		// write out the new particle:
		particleBuffer[newParticleIndex] = particle;

		// and add index to the alive list (push):
		uint aliveCount;
		counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_ALIVECOUNT, 1, aliveCount);
		aliveBuffer_CURRENT[aliveCount] = newParticleIndex;
	}
}
