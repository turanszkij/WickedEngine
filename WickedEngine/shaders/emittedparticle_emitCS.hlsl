#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "shadowHF.hlsli"

RWStructuredBuffer<Particle> particleBuffer : register(u0);
RWStructuredBuffer<uint> aliveBuffer_CURRENT : register(u1);
RWStructuredBuffer<uint> aliveBuffer_NEW : register(u2);
RWStructuredBuffer<uint> deadBuffer : register(u3);
RWByteAddressBuffer counterBuffer : register(u4);

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
	float3 nor = 0;
	float3 velocity = xParticleVelocity;
	float4 baseColor = EmitterGetMaterial().baseColor;
		
#ifdef EMIT_FROM_MESH
	// random subset of emitter mesh:
	uint geometryIndex = xEmitterMeshGeometryOffset + rng.next_uint(xEmitterMeshGeometryCount);
	ShaderGeometry geometry = load_geometry(geometryIndex);
	
	// random triangle on emitter surface:
	const uint triangleCount = geometry.indexCount / 3;
	const uint tri = rng.next_uint(triangleCount);

	// load indices of triangle from index buffer
	uint i0 = 0;
	uint i1 = 0;
	uint i2 = 0;
	[branch]
	if(geometry.ib >= 0)
	{
		Buffer<uint> buf = bindless_buffers_uint[geometry.ib];
		i0 = buf[geometry.indexOffset + tri * 3 + 0];
		i1 = buf[geometry.indexOffset + tri * 3 + 1];
		i2 = buf[geometry.indexOffset + tri * 3 + 2];
	}

	// load vertices of triangle from vertex buffer:
	float3 pos0 = 0;
	float3 pos1 = 0;
	float3 pos2 = 0;
	[branch]
	if(geometry.vb_pos_wind >= 0)
	{
		Buffer<float4> buf = bindless_buffers_float4[geometry.vb_pos_wind];
		pos0 = buf[i0].xyz;
		pos1 = buf[i1].xyz;
		pos2 = buf[i2].xyz;
	}

	float3 nor0 = 0;
	float3 nor1 = 0;
	float3 nor2 = 0;
	[branch]
	if(geometry.vb_nor >= 0)
	{
		Buffer<float4> buf = bindless_buffers_float4[geometry.vb_nor];
		nor0 = buf[i0].xyz;
		nor1 = buf[i1].xyz;
		nor2 = buf[i2].xyz;
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
	float2 bary = float2(f, g);

	// compute final surface position on triangle from barycentric coords:
	emitPos = attribute_at_bary(pos0.xyz, pos1.xyz, pos2.xyz, bary);
	emitPos = mul(xEmitterBaseMeshUnormRemap.GetMatrix(), float4(emitPos, 1)).xyz;
	nor = normalize(attribute_at_bary(nor0, nor1, nor2, bary));
	nor = normalize(mul((float3x3)worldMatrix, nor));

	// Take velocity from surface motion:
	[branch]
	if(geometry.vb_pre >= 0)
	{
		Buffer<float4> buf = bindless_buffers_float4[geometry.vb_pre];
		float3 pre0 = buf[i0].xyz;
		float3 pre1 = buf[i1].xyz;
		float3 pre2 = buf[i2].xyz;
		float3 pre = attribute_at_bary(pre0.xyz, pre1.xyz, pre2.xyz, bary);
		pre = mul(xEmitterBaseMeshUnormRemap.GetMatrix(), float4(pre, 1)).xyz;
		velocity += emitPos - pre;
	}

	if(xEmitterOptions & EMITTER_OPTION_BIT_TAKE_COLOR_FROM_MESH)
	{
		ShaderMaterial material = load_material(geometry.materialIndex);
		baseColor *= material.baseColor;
		
		[branch]
		if (geometry.vb_col >= 0 && material.IsUsingVertexColors())
		{
			Buffer<float4> buf = bindless_buffers_float4[geometry.vb_col];
			const float4 c0 = buf[i0];
			const float4 c1 = buf[i1];
			const float4 c2 = buf[i2];
			float4 vertexColor = attribute_at_bary(c0, c1, c2, bary);
			baseColor *= vertexColor;
		}
		
		float4 uvsets = 0;
		[branch]
		if(geometry.vb_uvs >= 0)
		{
			Buffer<float4> buf = bindless_buffers_float4[geometry.vb_uvs];
			float4 uv0 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i0]);
			float4 uv1 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i1]);
			float4 uv2 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i2]);
			uvsets = attribute_at_bary(uv0, uv1, uv2, bary);
			uvsets.xy = mad(uvsets.xy, material.texMulAdd.xy, material.texMulAdd.zw);

			[branch]
			if(material.textures[BASECOLORMAP].IsValid())
			{
				baseColor *= material.textures[BASECOLORMAP].SampleLevel(sampler_linear_wrap, uvsets, 0);
			}
		}
	}

#else

#ifdef EMITTER_VOLUME
	// Emit inside volume:
	emitPos = float3(rng.next_float() * 2 - 1, rng.next_float() * 2 - 1, rng.next_float() * 2 - 1);
#else
	// Just emit from center point:
	emitPos = 0;
#endif // EMITTER_VOLUME

#endif // EMIT_FROM_MESH

	float3 pos = mul(worldMatrix, float4(emitPos, 1)).xyz;
	
	// Blocker shadow map check using previous frame:
	[branch]
	if ((xEmitterOptions & EMITTER_OPTION_BIT_USE_RAIN_BLOCKER) && rain_blocker_check_prev(pos))
	{
		return;
	}

	float particleStartingSize = xParticleSize + xParticleSize * (rng.next_float() - 0.5f) * xParticleRandomFactor;

	// create new particle:
	Particle particle;
	particle.position = pos;
	particle.force = 0;
	particle.mass = xParticleMass;
	particle.velocity = velocity + (nor + (float3(rng.next_float(), rng.next_float(), rng.next_float()) - 0.5f) * xParticleRandomFactor) * xParticleNormalFactor;
	particle.rotationalVelocity = xParticleRotation + (rng.next_float() - 0.5f) * xParticleRandomFactor;
	particle.maxLife = xParticleLifeSpan + xParticleLifeSpan * (rng.next_float() - 0.5f) * xParticleLifeSpanRandomness;
	particle.life = particle.maxLife;
	particle.sizeBeginEnd = float2(particleStartingSize, particleStartingSize * xParticleScaling);

	baseColor.r *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
	baseColor.g *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
	baseColor.b *= lerp(1, rng.next_float(), xParticleRandomColorFactor);
	particle.color = pack_rgba(baseColor);
	
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
