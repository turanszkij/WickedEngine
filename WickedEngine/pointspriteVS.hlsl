#include "globals.hlsli"

CBUFFER(EmittedParticleCB, CBSLOT_OTHER_EMITTEDPARTICLE)
{
	float2		xAdd;
	float		xMotionBlurAmount;
	float		padding;
};


// 2;3  5
// +----+
// |\   |
// | \  |
// |  \ |
// |   \|
// +----+
// 0    1;4
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

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float4 opaAddDarkSiz	: TEXCOORD1;
	float4 pp				: TEXCOORD2;
};

VertextoPixel main(uint fakeIndex : SV_VERTEXID)
{
	VertextoPixel Out;

	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;
	Particle particle = particleBuffer[instanceID];

	float3 quadPos = BILLBOARD[vertexID];

	float2 uv = quadPos.xy * 0.5f + 0.5f;
	uv.x = particle.sizOpaMir.z > 0 ? 1.0f - uv.x : uv.x;
	uv.y = particle.sizOpaMir.w > 0 ? 1.0f - uv.y : uv.y;
	
	float2x2 rot = float2x2(
		cos(particle.rot), -sin(particle.rot),
		sin(particle.rot), cos(particle.rot)
		);
	quadPos.xy = mul(quadPos.xy, rot);

	float quadLength = particle.sizOpaMir.x;
	quadPos *= quadLength;

	float3 velocity = mul(particle.vel, (float3x3)g_xCamera_View);
	quadPos += normalize(velocity * dot(quadPos, velocity)) * xMotionBlurAmount;


	Out.pos = float4(particle.pos, 1);
	Out.pos = mul(Out.pos, g_xCamera_View);
	Out.pos.xyz += quadPos.xyz * particle.sizOpaMir.x;
	Out.pos = mul(Out.pos, g_xCamera_Proj);

	Out.tex = uv;
	Out.opaAddDarkSiz = float4(saturate(particle.sizOpaMir.y), xAdd.x, xAdd.y, particle.sizOpaMir.x);
	Out.pp = Out.pos;

	return Out;
}