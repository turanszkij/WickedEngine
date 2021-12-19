#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "emittedparticleHF.hlsli"

static const float3 BILLBOARD[] = {
    float3(-1, -1, 0),	// 0
    float3(1, -1, 0),	// 1
    float3(-1, 1, 0),	// 2
    float3(1, 1, 0),	// 3
};
static const uint BILLBOARD_VERTEXCOUNT = 4;

ByteAddressBuffer counterBuffer : register(t0);
StructuredBuffer<Particle> particleBuffer : register(t1);
StructuredBuffer<uint> culledIndirectionBuffer : register(t2);
StructuredBuffer<uint> culledIndirectionBuffer2 : register(t3);

static const uint VERTEXCOUNT = THREADCOUNT_MESH_SHADER * BILLBOARD_VERTEXCOUNT;
static const uint PRIMITIVECOUNT = THREADCOUNT_MESH_SHADER * 2;

[outputtopology("triangle")]
[numthreads(THREADCOUNT_MESH_SHADER, 1, 1)]
void main(
    in uint tid : SV_DispatchThreadID,
    in uint tig : SV_GroupIndex,
	in uint gid : SV_GroupID,
    out vertices VertextoPixel verts[VERTEXCOUNT],
    out indices uint3 triangles[PRIMITIVECOUNT])
{
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_CULLEDCOUNT);
	uint realGroupCount = min(THREADCOUNT_MESH_SHADER, particleCount - gid * THREADCOUNT_MESH_SHADER);

    // Set number of outputs
    SetMeshOutputCounts(realGroupCount * BILLBOARD_VERTEXCOUNT, realGroupCount * 2);

	if (tig >= realGroupCount)
		return;

	ShaderMesh mesh = EmitterGetMesh();

	uint instanceID = tid;
	uint particleIndex = culledIndirectionBuffer2[culledIndirectionBuffer[instanceID]];

	// load particle data:
	Particle particle = particleBuffer[particleIndex];

	// calculate render properties from life:
	float lifeLerp = 1 - particle.life / particle.maxLife;
	float size = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

	// Sprite sheet UV transform:
	const float spriteframe = xEmitterFrameRate == 0 ?
		lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) :
		((xEmitterFrameStart + particle.life * xEmitterFrameRate) % xEmitterFrameCount);
	const float frameBlend = frac(spriteframe);

    // Transform the vertices and write them
	for (uint i = 0; i < BILLBOARD_VERTEXCOUNT; ++i)
	{
		uint vertexID = particleIndex * 4 + i;

		uint4 data = bindless_buffers[mesh.vb_pos_nor_wind].Load4(vertexID * 16);
		float3 position = asfloat(data.xyz);
		float3 normal = normalize(unpack_unitvector(data.w));
		float2 uv = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(vertexID * 4));
		float2 uv2 = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(vertexID * 4));
		uint color = bindless_buffers[mesh.vb_col].Load(vertexID * 4);

		VertextoPixel Out;
		Out.P = position;
		Out.pos = mul(GetCamera().view_projection, float4(position, 1));
		Out.tex = float4(uv, uv2);
		Out.size = size;
		Out.color = color;
		Out.unrotated_uv = BILLBOARD[i].xy * float2(1, -1) * 0.5f + 0.5f;
		Out.frameBlend = frameBlend;

		verts[tig * BILLBOARD_VERTEXCOUNT + i] = Out;
	}

	triangles[tig * 2 + 0] = uint3(0, 1, 2) + tig * BILLBOARD_VERTEXCOUNT;
	triangles[tig * 2 + 1] = uint3(2, 1, 3) + tig * BILLBOARD_VERTEXCOUNT;
}
