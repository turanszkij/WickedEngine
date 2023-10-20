#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "shadowHF.hlsli"

RWStructuredBuffer<Particle> particleBuffer : register(u0);
RWStructuredBuffer<uint> aliveBuffer_CURRENT : register(u1);
RWStructuredBuffer<uint> aliveBuffer_NEW : register(u2);
RWStructuredBuffer<uint> deadBuffer : register(u3);
RWByteAddressBuffer counterBuffer : register(u4);

#ifdef EMIT_FROM_MESH
Buffer<uint> meshIndexBuffer : register(t0);
Buffer<float4> meshVertexBuffer_POS : register(t1);
#endif // EMIT_FROM_MESH


[numthreads(THREADCOUNT_EMIT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint emitCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_REALEMITCOUNT);
	if (DTid.x >= emitCount)
		return;
	
	RNG rng;
	rng.init(uint2(xEmitterRandomness, DTid.x), GetFrame().frame_count);

	const float4x4 worldMatrix = xEmitterTransform.GetMatrix();
	float3 emitPos = 0;
		
#ifdef EMIT_FROM_MESH
	// random triangle on emitter surface:
	const uint triangleCount = xEmitterMeshIndexCount / 3;
	const uint tri = rng.next_uint(triangleCount);

	// load indices of triangle from index buffer
	uint i0 = meshIndexBuffer[tri * 3 + 0];
	uint i1 = meshIndexBuffer[tri * 3 + 1];
	uint i2 = meshIndexBuffer[tri * 3 + 2];

	// load vertices of triangle from vertex buffer:
	float4 pos_nor0 = meshVertexBuffer_POS[i0];
	float4 pos_nor1 = meshVertexBuffer_POS[i1];
	float4 pos_nor2 = meshVertexBuffer_POS[i2];
	
	float3 nor0 = unpack_unitvector(asuint(pos_nor0.w));
	float3 nor1 = unpack_unitvector(asuint(pos_nor1.w));
	float3 nor2 = unpack_unitvector(asuint(pos_nor2.w));

	// random barycentric coords:
	float f = rng.next_float();
	float g = rng.next_float();
	[flatten]
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}
	float2 bary = float2(f, g);

	// compute final surface position on triangle from barycentric coords:
	emitPos = attribute_at_bary(pos_nor0.xyz, pos_nor1.xyz, pos_nor2.xyz, bary);
	float3 nor = normalize(attribute_at_bary(nor0, nor1, nor2, bary));
	nor = normalize(mul((float3x3)worldMatrix, nor));

#else

#ifdef EMITTER_VOLUME
	// Emit inside volume:
	emitPos = float3(rng.next_float() * 2 - 1, rng.next_float() * 2 - 1, rng.next_float() * 2 - 1);
#else
	// Just emit from center point:
	emitPos = 0;
#endif // EMITTER_VOLUME

	float3 nor = 0;

#endif // EMIT_FROM_MESH

	float3 pos = mul(worldMatrix, float4(emitPos, 1)).xyz;
	
	// Blocker shadow map check:
	[branch]
	if ((xEmitterOptions & EMITTER_OPTION_BIT_USE_RAIN_BLOCKER) && rain_blocker_check(pos))
	{
		return;
	}

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
	color_modifier |= (uint) (255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 0;
	color_modifier |= (uint) (255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 8;
	color_modifier |= (uint) (255.0 * lerp(1, rng.next_float(), xParticleRandomColorFactor)) << 16;
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
