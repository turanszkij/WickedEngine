#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

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

STRUCTUREDBUFFER(particleBuffer, Patch, 0);

VertexToPixel main(uint fakeIndex : SV_VERTEXID)
{
	VertexToPixel Out;

	// bypass the geometry shader and expand the particle here:
	const uint vertexID = fakeIndex % 12;
	const uint segmentID = (fakeIndex / 12) % xHairSegmentCount;
	const uint particleID = fakeIndex / 12;

	float3 position = particleBuffer[particleID].position;
	Out.fade = saturate(distance(position.xyz, g_xCamera_CamPos.xyz) / xHairViewDistance);
	Out.fade = saturate(Out.fade - 0.8f) * 5.0f; // fade will be on edge and inwards 20%

	[branch]
	if (Out.fade < 1.0 - 1.0f / 255.0f)
	{
		uint tangent_random = particleBuffer[particleID].tangent_random;
		float3 normal = particleBuffer[particleID].normal;
		uint binormal_length = particleBuffer[particleID].binormal_length;

		float3 tangent;
		tangent.x = (tangent_random >> 0) & 0x000000FF;
		tangent.y = (tangent_random >> 8) & 0x000000FF;
		tangent.z = (tangent_random >> 16) & 0x000000FF;
		tangent = tangent / 255.0f * 2 - 1;

		uint rand = (tangent_random >> 24) & 0x000000FF;

		float3 binormal;
		binormal.x = (binormal_length >> 0) & 0x000000FF;
		binormal.y = (binormal_length >> 8) & 0x000000FF;
		binormal.z = (binormal_length >> 16) & 0x000000FF;
		binormal = binormal / 255.0f * 2 - 1;

		float len = (binormal_length >> 24) & 0x000000FF;
		len /= 255.0f;
		len += 1;
		len *= xLength;

		// expand the particle into a billboard cross section, the patch:
		float3 patchPos = HAIRPATCH[vertexID];
		float2 uv = vertexID < 6 ? patchPos.xy : patchPos.zy;
		uv = uv * float2(0.5f, 0.5f) + 0.5f;
		uv.y = lerp((float)segmentID / (float)xHairSegmentCount, ((float)segmentID + 1) / (float)xHairSegmentCount, uv.y);
		uv.y = 1 - uv.y;
		uv.x = rand % 2 == 0 ? uv.x : 1 - uv.x;
		patchPos.y += 1;

		// scale the billboard by the texture aspect:
		float2 frame;
		texture_0.GetDimensions(frame.x, frame.y);
		frame.xy /= frame.y;
		frame.xy *= len;
		patchPos.xyz *= frame.xyx * 0.5f;

		// simplistic wind effect only affects the top, but leaves the base as is:
		float3 wind = sin(g_xFrame_Time + (position.x + position.y + position.z)) * g_xFrame_WindDirection.xyz * patchPos.y * 0.03f;
		float3 windPrev = sin(g_xFrame_TimePrev + (position.x + position.y + position.z)) * g_xFrame_WindDirection.xyz * patchPos.y * 0.03f;

		// rotate the patch into the tangent space of the emitting triangle:
		float3x3 TBN = float3x3(tangent, normal, binormal); // don't derive binormal, because we want the shear!
		patchPos = mul(patchPos, TBN);

		// inset to the emitter a bit, to avoid disconnect:
		position.xyz -= normal * 0.1 * len;


		// copy to output:
		Out.pos = float4(position, 1);
		Out.pos.xyz += patchPos;
		float3 savedPos = Out.pos.xyz;
		Out.pos.xyz += wind;
		Out.pos3D = Out.pos.xyz;
		Out.pos = mul(g_xCamera_VP, Out.pos);

		Out.nor = normal;
		Out.tex = uv;

		Out.pos2D = Out.pos;
		Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, float4(savedPos + windPrev, 1));

		Out.color = xColor.rgb;
	}
	else
	{
		Out = (VertexToPixel)0;
	}

	return Out;
}
