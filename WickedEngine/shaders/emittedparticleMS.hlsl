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

// VertexToPixel separated into two structures:
struct VertextoPixel_MS
{
	float4 pos : SV_POSITION;
	float clip : SV_ClipDistance0;
	half4 tex : TEXCOORD0;
	float3 P : WORLDPOSITION;
	half2 unrotated_uv : UNROTATED_UV;
};
struct VertextoPixel_MS_PRIM
{
	nointerpolation half frameBlend : FRAMEBLEND;
	nointerpolation half size : PARTICLESIZE;
	nointerpolation uint color : PARTICLECOLOR;
};

[outputtopology("triangle")]
[numthreads(THREADCOUNT_MESH_SHADER, 1, 1)]
void main(
    in uint tid : SV_DispatchThreadID,
    in uint tig : SV_GroupIndex,
	in uint gid : SV_GroupID,
    out vertices VertextoPixel_MS verts[VERTEXCOUNT],
	out primitives VertextoPixel_MS_PRIM sharedPrimitives[PRIMITIVECOUNT],
    out indices uint3 triangles[PRIMITIVECOUNT])
{
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_CULLEDCOUNT);
	uint realGroupCount = min(THREADCOUNT_MESH_SHADER, particleCount - gid * THREADCOUNT_MESH_SHADER);

    // Set number of outputs
    SetMeshOutputCounts(realGroupCount * BILLBOARD_VERTEXCOUNT, realGroupCount * 2);

	if (tig >= realGroupCount)
		return;
	
	ShaderGeometry geometry = EmitterGetGeometry();

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

		float3 position = bindless_buffers_float4[geometry.vb_pos_wind][vertexID].xyz;
		float3 normal = normalize(bindless_buffers_float4[geometry.vb_nor][vertexID].xyz);
		float4 uvsets = bindless_buffers_float4[geometry.vb_uvs][vertexID];

		VertextoPixel_MS Out;
		Out.P = position;
		Out.clip = dot(Out.pos, GetCamera().clip_plane);
		Out.tex = half4(uvsets);
		Out.pos = mul(GetCamera().view_projection, float4(position, 1));
		Out.unrotated_uv = half2(BILLBOARD[i].xy * float2(1, -1) * 0.5f + 0.5f);

		verts[tig * BILLBOARD_VERTEXCOUNT + i] = Out;
	}

	VertextoPixel_MS_PRIM OutQuad;
	OutQuad.color = pack_rgba(bindless_buffers_float4[geometry.vb_col][particleIndex * 4]);
	OutQuad.size = half(size);
	OutQuad.frameBlend = half(frameBlend);
	sharedPrimitives[tig * 2 + 0] = OutQuad;
	sharedPrimitives[tig * 2 + 1] = OutQuad;

	triangles[tig * 2 + 0] = uint3(0, 1, 2) + tig * BILLBOARD_VERTEXCOUNT;
	triangles[tig * 2 + 1] = uint3(2, 1, 3) + tig * BILLBOARD_VERTEXCOUNT;
}
