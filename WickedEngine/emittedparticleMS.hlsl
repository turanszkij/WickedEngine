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

RAWBUFFER(counterBuffer, TEXSLOT_ONDEMAND20);
STRUCTUREDBUFFER(particleBuffer, Particle, TEXSLOT_ONDEMAND21);
STRUCTUREDBUFFER(aliveList, uint, TEXSLOT_ONDEMAND22);

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
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION);
	uint realGroupCount = min(THREADCOUNT_MESH_SHADER, particleCount - gid * THREADCOUNT_MESH_SHADER);

    // Set number of outputs
    SetMeshOutputCounts(realGroupCount * BILLBOARD_VERTEXCOUNT, realGroupCount * 2);

	if (tig >= realGroupCount)
		return;

	uint instanceID = tid;

	// load particle data:
	Particle particle = particleBuffer[aliveList[instanceID]];

	// calculate render properties from life:
	float lifeLerp = 1 - particle.life / particle.maxLife;
	float size = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);
	float opacity = saturate(lerp(1, 0, lifeLerp) * xEmitterOpacity);
	float rotation = lifeLerp * particle.rotationalVelocity;

	const float spriteframe = xEmitterFrameRate == 0 ?
		lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) :
		((xEmitterFrameStart + particle.life * xEmitterFrameRate) % xEmitterFrameCount);
	const uint currentFrame = floor(spriteframe);
	const uint nextFrame = ceil(spriteframe);
	const float frameBlend = frac(spriteframe);
	uint2 offset = uint2(currentFrame % xEmitterFramesXY.x, currentFrame / xEmitterFramesXY.x);
	uint2 offset2 = uint2(nextFrame % xEmitterFramesXY.x, nextFrame / xEmitterFramesXY.x);

	float2x2 rot = float2x2(
		cos(rotation), -sin(rotation),
		sin(rotation), cos(rotation)
		);

	float3 velocity = mul((float3x3)g_xCamera_View, particle.velocity);

    // Transform the vertices and write them
	for (uint i = 0; i < BILLBOARD_VERTEXCOUNT; ++i)
	{
		VertextoPixel Out;
		uint vertexID = i;

		// expand the point into a billboard in view space:
		float3 quadPos = BILLBOARD[vertexID];
		quadPos.x = particle.color_mirror & 0x10000000 ? -quadPos.x : quadPos.x;
		quadPos.y = particle.color_mirror & 0x20000000 ? -quadPos.y : quadPos.y;
		float2 uv = quadPos.xy * float2(0.5f, -0.5f) + 0.5f;
		float2 uv2 = uv;

		// Sprite sheet UV transform:
		uv.xy += offset;
		uv.xy *= xEmitterTexMul;
		uv2.xy += offset2;
		uv2.xy *= xEmitterTexMul;

		// rotate the billboard:
		quadPos.xy = mul(quadPos.xy, rot);

		// scale the billboard:
		quadPos *= size;

		// scale the billboard along view space motion vector:
		quadPos += dot(quadPos, velocity) * velocity * xParticleMotionBlurAmount;


		// copy to output:
		Out.pos = float4(particle.position, 1);
		Out.pos = mul(g_xCamera_View, Out.pos);
		Out.pos.xyz += quadPos.xyz;
		Out.P = mul(g_xCamera_InvV, float4(Out.pos.xyz, 1)).xyz;
		Out.pos = mul(g_xCamera_Proj, Out.pos);

#ifdef SPIRV
		Out.pos.y = -Out.pos.y;
#endif // SPIRV

		Out.tex = float4(uv, uv2);
		Out.size = size;
		Out.color = (particle.color_mirror & 0x00FFFFFF) | (uint(opacity * 255.0f) << 24);
		Out.pos2D = Out.pos;
		Out.unrotated_uv = quadPos.xy * float2(1, -1) / size * 0.5f + 0.5f;
		Out.frameBlend = frameBlend;


		verts[tig * BILLBOARD_VERTEXCOUNT + i] = Out;
	}

	triangles[tig * 2 + 0] = uint3(0, 1, 2) + tig * BILLBOARD_VERTEXCOUNT;
	triangles[tig * 2 + 1] = uint3(2, 1, 3) + tig * BILLBOARD_VERTEXCOUNT;
}
