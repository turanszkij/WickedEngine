#include "globals.hlsli"
#include "hairparticleHF.hlsli"

static const float hairPopDistanceThreshold = 0.9f;

// billboard cross section:
static const float3 HAIRPATCH[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(-1, 1, 0),
	float3(1, -1, 0),
	float3(1, 1, 0),

	float3(0, -1, -1),
	float3(0, -1, 1),
	float3(0, 1, -1),
	float3(0, 1, -1),
	float3(0, -1, 1),
	float3(0, 1, 1),
};

static const uint particleBuffer_stride = 16 + 4 + 4; // pos, normal, tangent 
RAWBUFFER(particleBuffer, 0);

VertexToPixel main(uint fakeIndex : SV_VERTEXID)
{
	VertexToPixel Out;

	// bypass the geometry shader and expand the particle here:
	uint vertexID = fakeIndex % 12;
	uint instanceID = fakeIndex / 12;

	// load the raw particle data:
	const uint fetchAddress = instanceID * particleBuffer_stride;
	float4 posLen = asfloat(particleBuffer.Load4(fetchAddress));
	uint normalRand = particleBuffer.Load(fetchAddress + 16);
	uint uTangent = particleBuffer.Load(fetchAddress + 16 + 4);

	// convert the raw loaded particle data:
	float3 pos = posLen.xyz;
	float len = posLen.w;
	float3 normal;
	uint rand;
	normal.x = (normalRand >> 0) & 0x000000FF;
	normal.y = (normalRand >> 8) & 0x000000FF;
	normal.z = (normalRand >> 16) & 0x000000FF;
	normal = normal / 255.0f * 2 - 1;
	rand = (normalRand >> 24) & 0x000000FF;
	float3 tangent;
	tangent.x = (uTangent >> 0) & 0x000000FF;
	tangent.y = (uTangent >> 8) & 0x000000FF;
	tangent.z = (uTangent >> 16) & 0x000000FF;
	tangent = tangent / 255.0f * 2 - 1;

	// expand the particle into a billboard cross section, the patch:
	float3 patchPos = HAIRPATCH[vertexID];
	float2 uv = vertexID < 6 ? patchPos.xy : patchPos.zy;
	uv = uv * float2(0.5f, -0.5f) + 0.5f;
	uv.x = rand % 2 == 0 ? uv.x : 1 - uv.x;
	patchPos.y += 1;

	// scale the billboard by the texture aspect:
	float2 frame;
	texture_0.GetDimensions(frame.x, frame.y);
	frame.xy /= frame.y;
	frame.xy *= len;
	patchPos.xyz *= frame.xyx * 0.5f;

	// simplistic wind effect only affects the top, but leaves the base as is:
	float3 wind = sin(g_xFrame_Time + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz * patchPos.y * 0.03f;
	float3 windPrev = sin(g_xFrame_TimePrev + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz * patchPos.y * 0.03f;

	// transform particle by the emitter object matrix:
	pos.xyz = mul(float4(pos.xyz, 1), xWorld).xyz;
	normal = mul(normal, (float3x3)xWorld);
	tangent = mul(tangent, (float3x3)xWorld);

	// rotate the patch into the tangent space of the emitting triangle:
	float3x3 TBN = float3x3(tangent, normal, cross(normal, tangent));
	patchPos = mul(patchPos, TBN);

	// inset to the emitter a bit, to avoid disconnect:
	pos.xyz -= normal*0.1*len;


	// copy to output:
	Out.pos = float4(pos, 1);
	Out.pos.xyz += patchPos;
	float3 savedPos = Out.pos.xyz;
	Out.pos.xyz += wind;
	Out.pos3D = Out.pos.xyz;
	Out.pos = mul(Out.pos, g_xCamera_VP);

	Out.nor = normal;
	Out.tex = uv; 
	
	Out.fade = pow(saturate(distance(pos.xyz, g_xCamera_CamPos.xyz) / (LOD2*hairPopDistanceThreshold)), 10);
	Out.pos2D = Out.pos;
	Out.pos2DPrev = mul(float4(savedPos + windPrev, 1), g_xFrame_MainCamera_PrevVP);

	return Out;
}
