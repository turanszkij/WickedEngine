#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWStructuredBuffer<Particle> particleBuffer : register(u0);
RWStructuredBuffer<uint> aliveBuffer_CURRENT : register(u1);
RWStructuredBuffer<uint> aliveBuffer_NEW : register(u2);
RWStructuredBuffer<uint> deadBuffer : register(u3);
RWByteAddressBuffer counterBuffer : register(u4);

#ifdef EMIT_FROM_MESH
Buffer<uint> meshIndexBuffer : register(t0);
ByteAddressBuffer meshVertexBuffer_POS : register(t1);
#endif // EMIT_FROM_MESH


[numthreads(THREADCOUNT_EMIT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint emitCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_REALEMITCOUNT);

	if(DTid.x < emitCount)
	{
		RNG rng;
		rng.init(uint2(xEmitterRandomness, DTid.x), GetFrame().frame_count);

		const float4x4 worldMatrix = xEmitterTransform.GetMatrix();
		
#ifdef EMIT_FROM_MESH
		// random triangle on emitter surface:
		uint tri = (uint)((xEmitterMeshIndexCount / 3) * hash1(DTid.x + GetFrame().frame_count));

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
		float f = rng.next_float();
		float g = rng.next_float();
		[flatten]
		if (f + g > 1)
		{
			f = 1 - f;
			g = 1 - g;
		}

		// compute final surface position on triangle from barycentric coords:
		float3 pos = pos_nor0.xyz + f * (pos_nor1.xyz - pos_nor0.xyz) + g * (pos_nor2.xyz - pos_nor0.xyz);
		float3 nor = nor0 + f * (nor1 - nor0) + g * (nor2 - nor0);
		pos = mul(worldMatrix, float4(pos, 1)).xyz;
		nor = normalize(mul((float3x3)worldMatrix, nor));

#else

#ifdef EMITTER_VOLUME
		// Emit inside volume:
		float3 pos = mul(worldMatrix, float4(rng.next_float() * 2 - 1, rng.next_float() * 2 - 1, rng.next_float() * 2 - 1, 1)).xyz;
#else
		// Just emit from center point:
		float3 pos = mul(worldMatrix, float4(0, 0, 0, 1)).xyz;
#endif // EMITTER_VOLUME

		float3 nor = 0;

#endif // EMIT_FROM_MESH

		float particleStartingSize = xParticleSize + xParticleSize * (rng.next_float() - 0.5f) * xParticleRandomFactor;

		// create new particle:
		Particle particle;
		particle.position = pos;
		particle.force = 0;
		particle.mass = xParticleMass;
		particle.velocity = xParticleVelocity + (nor + (float3(rng.next_float(), rng.next_float(), rng.next_float()) - 0.5f) * xParticleRandomFactor) * xParticleNormalFactor;
		particle.rotationalVelocity = xParticleRotation + (rng.next_float() - 0.5f) * xParticleRandomFactor;
		particle.maxLife = xParticleLifeSpan + xParticleLifeSpan * (rng.next_float() - 0.5f) * xParticleLifeSpanRandomness;
		particle.life = particle.maxLife;
		particle.sizeBeginEnd = float2(particleStartingSize, particleStartingSize * xParticleScaling);
		particle.color_mirror = 0;
		particle.color_mirror |= ((rng.next_float() > 0.5f) << 31) & 0x10000000;
		particle.color_mirror |= ((rng.next_float() < 0.5f) << 30) & 0x20000000;

		uint color_modifier = 0;
		color_modifier |= (uint)(255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 0;
		color_modifier |= (uint)(255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 8;
		color_modifier |= (uint)(255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 16;
		particle.color_mirror |= pack_rgba(float4(EmitterGetMaterial().baseColor.rgb, 1)) & color_modifier;


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
