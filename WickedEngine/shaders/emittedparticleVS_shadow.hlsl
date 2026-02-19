#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "emittedparticleHF.hlsli"

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};

StructuredBuffer<Particle> particleBuffer : register(t1);
StructuredBuffer<uint> aliveList : register(t2);

VertextoPixel main(uint vid : SV_VertexID, uint instanceID : SV_InstanceID)
{
	ShaderGeometry geometry = EmitterGetGeometry();

	uint particleIndex = aliveList[instanceID];
	uint vertexID = particleIndex * 4 + vid;

	// load particle data:
	Particle particle = particleBuffer[particleIndex];

	const float dt = xEmitterFixedTimestep >= 0 ? xEmitterFixedTimestep : GetFrame().delta_time;
	float2 rotation_rotationVel = unpack_half2(particle.rotation_rotationVelocity);
	rotation_rotationVel.x += rotation_rotationVel.y * dt;
	float rotation = rotation_rotationVel.x;
	float2x2 rot = float2x2(
		cos(rotation), -sin(rotation),
		sin(rotation), cos(rotation)
		);

	// calculate render properties from life:
	float lifeLerp = 1 - particle.life / particle.maxLife;
	float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

	// Construct camera facing quad here instead of using maincamera vertex positions:
	float3 quadPos = BILLBOARD[vid];

	// rotate the billboard:
	quadPos.xy = mul(quadPos.xy, rot);

	// scale the billboard:
	quadPos *= particleSize;

	// scale the billboard along view space motion vector:
	float3 velocity = mul((float3x3)GetCamera().view, particle.velocity);
	quadPos += dot(quadPos, velocity) * velocity * xParticleMotionBlurAmount;

	// rotate the billboard to face the camera:
	quadPos = mul(quadPos, (float3x3)GetCamera().view); // reversed mul for inverse camera rotation!

	float3 position = particle.position + quadPos;
	//float3 position = bindless_buffers_float4[descriptor_index(geometry.vb_pos_wind)][vertexID].xyz;
	float3 normal = normalize(bindless_buffers_float4[descriptor_index(geometry.vb_nor)][vertexID].xyz);
	float4 uvsets = bindless_buffers_float4[descriptor_index(geometry.vb_uvs)][vertexID];
	float4 color = bindless_buffers_float4[descriptor_index(geometry.vb_col)][vertexID];

	// Sprite sheet UV transform:
	const float spriteframe = xEmitterFrameRate == 0 ? 
		lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) : 
		((xEmitterFrameStart + (particle.maxLife - particle.life) * xEmitterFrameRate) % xEmitterFrameCount);
	const float frameBlend = frac(spriteframe);

	VertextoPixel Out;
	Out.pos = float4(position, 1);
	Out.clip = dot(Out.pos, GetCamera().clip_plane);
	Out.pos = mul(GetCamera().view_projection, Out.pos);
	Out.tex = uvsets;
	Out.size = half(particleSize);
	Out.color = pack_rgba(color);
	Out.unrotated_uv = half2(BILLBOARD[vertexID % 4].xy * float2(1, -1) * 0.5f + 0.5f);
	Out.frameBlend = half(frameBlend);
	return Out;
}
