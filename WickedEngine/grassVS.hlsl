#include "globals.hlsli"

//struct VS_OUT
//{
//	float4 pos : SV_POSITION;
//	float4 nor : NORMAL;
//	float4 tan : TANGENT;
//};

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float3 xColor; float __pad0;
	float LOD0;
	float LOD1;
	float LOD2;
	float __pad1;
}


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

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float  fade : DITHERFADE;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

VertexToPixel main(uint fakeIndex : SV_VERTEXID)
{
	VertexToPixel Out;

	uint vertexID = fakeIndex % 12;
	uint instanceID = fakeIndex / 12;

	const uint fetchAddress = instanceID * particleBuffer_stride;
	float4 posLen = asfloat(particleBuffer.Load4(fetchAddress));
	uint normalRand = particleBuffer.Load(fetchAddress + 16);
	uint uTangent = particleBuffer.Load(fetchAddress + 16 + 4);

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

	float3 patchPos = HAIRPATCH[vertexID];
	float2 uv = vertexID < 6 ? patchPos.xy : patchPos.zy;
	uv = uv * float2(0.5f, -0.5f) + 0.5f;
	patchPos.y += 1;


	float3 wind = sin(g_xFrame_Time + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz*0.03*len;
	float3 windPrev = sin(g_xFrame_TimePrev + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz*0.03*len;


	float2 frame;
	texture_0.GetDimensions(frame.x, frame.y);
	frame.xy /= frame.y;
	frame.xy *= len;

	patchPos.xy *= frame * 0.5f;

	//float3x3 TBN = { tangent, cross(tangent, normal), normal };
	//patchPos = mul(patchPos, TBN);

	uv.x = rand % 2 == 0 ? uv.x : 1 - uv.x;

	pos.xyz -= normal*0.1*len;


	Out.pos = float4(pos, 1);
	Out.pos.xyz += patchPos;
	float3 savedPos = Out.pos.xyz;
	Out.pos.xyz += wind;
	Out.pos3D = Out.pos.xyz;
	Out.pos = mul(Out.pos, g_xCamera_VP);

	Out.nor = normal;
	Out.tex = uv; 
	
	static const float grassPopDistanceThreshold = 0.9f;
	Out.fade = pow(saturate(distance(pos.xyz, g_xCamera_CamPos.xyz) / (LOD2*grassPopDistanceThreshold)), 10);
	Out.pos2D = Out.pos;
	Out.pos2DPrev = mul(float4(savedPos + windPrev, 1), g_xFrame_MainCamera_PrevVP);

	return Out;
}
