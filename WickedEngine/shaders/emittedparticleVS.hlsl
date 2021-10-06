#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "emittedparticleHF.hlsli"

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};

STRUCTUREDBUFFER(particleBuffer, Particle, TEXSLOT_ONDEMAND21);
STRUCTUREDBUFFER(aliveList, uint, TEXSLOT_ONDEMAND22);

VertextoPixel main(uint vertexID : SV_VERTEXID)
{
	ShaderMesh mesh = EmitterGetMesh();

	VertextoPixel Out = (VertextoPixel)0;

	uint4 data = bindless_buffers[mesh.vb_pos_nor_wind].Load4(vertexID * 16);
	float3 position = asfloat(data.xyz);
	float3 normal = normalize(unpack_unitvector(data.w));
	float2 uv = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(vertexID * 4));
	float2 uv2 = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(vertexID * 4));
	uint color = bindless_buffers[mesh.vb_col].Load(vertexID * 4);


	// load particle data:
	Particle particle = particleBuffer[aliveList[vertexID / 4]];

	// calculate render properties from life:
	float lifeLerp = 1 - particle.life / particle.maxLife;
	float size = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

	// Sprite sheet UV transform:
	const float spriteframe = xEmitterFrameRate == 0 ? 
		lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) : 
		((xEmitterFrameStart + particle.life * xEmitterFrameRate) % xEmitterFrameCount);
	const float frameBlend = frac(spriteframe);

	Out.P = position;
	Out.pos = mul(GetCamera().VP, float4(position, 1));
	Out.tex = float4(uv, uv2);
	Out.size = size;
	Out.color = color;
	Out.unrotated_uv = BILLBOARD[vertexID % 4].xy * float2(1, -1) * 0.5f + 0.5f;
	Out.frameBlend = frameBlend;

	return Out;
}
