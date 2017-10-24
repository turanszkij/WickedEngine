#include "globals.hlsli"
#include "emittedparticleHF.hlsli"

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float2		xAdd;
	float		xMotionBlurAmount;
	uint		bufferOffset;
};

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(-1, 1, 0),	// 3
	float3(1, -1, 0),	// 4
	float3(1, 1, 0),	// 5
};

struct Particle
{
	float3 pos;
	float rot;
	float4 sizOpaMir;
	float3 vel;
	float rotVel;
	float life;
	float maxLife;
	float2 sizBeginEnd;
};

STRUCTUREDBUFFER(particleBuffer, Particle, 0);

VertextoPixel main(uint fakeIndex : SV_VERTEXID)
{
	VertextoPixel Out;

	// bypass the geometry shader, and instead expand the particle here in the VS:
	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;

	// load particle data:
	Particle particle = particleBuffer[instanceID + bufferOffset];

	// expand the point into a billboard in view space:
	float3 quadPos = BILLBOARD[vertexID];
	float2 uv = quadPos.xy * float2(0.5f, -0.5f) + 0.5f;
	uv.x = particle.sizOpaMir.z > 0 ? 1.0f - uv.x : uv.x;
	uv.y = particle.sizOpaMir.w > 0 ? 1.0f - uv.y : uv.y;
	
	// rotate the billboard:
	float2x2 rot = float2x2(
		cos(particle.rot), -sin(particle.rot),
		sin(particle.rot), cos(particle.rot)
		);
	quadPos.xy = mul(quadPos.xy, rot);

	// scale the billboard:
	float quadLength = particle.sizOpaMir.x;
	quadPos *= quadLength;

	// TODO: this will be removed when particle system editor is completed
	quadPos *= 0.05f;

	// scale the billboard along view space motion vector:
	float3 velocity = mul(particle.vel, (float3x3)g_xCamera_View);
	quadPos += dot(quadPos, velocity) * velocity * xMotionBlurAmount;


	// copy to output:
	Out.pos = float4(particle.pos, 1);
	Out.pos = mul(Out.pos, g_xCamera_View);
	Out.pos.xyz += quadPos.xyz * particle.sizOpaMir.x;
	Out.pos = mul(Out.pos, g_xCamera_Proj);

	Out.tex = uv;
	Out.opaAddDarkSiz = float4(saturate(particle.sizOpaMir.y), xAdd.x, xAdd.y, particle.sizOpaMir.x);
	Out.pos2D = Out.pos;

	return Out;
}