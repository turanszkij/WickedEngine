#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

static const float3 HAIRPATCH[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(1, 1, 0),
};

STRUCTUREDBUFFER(particleBuffer, Patch, 0);
STRUCTUREDBUFFER(culledIndexBuffer, uint, 1);

VertexToPixel main(uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	VertexToPixel Out;

	const uint particleID = culledIndexBuffer[instanceID];
	const uint segmentID = particleID % xHairSegmentCount;

	float3 rootposition = particleBuffer[particleID - segmentID].position;
	Out.fade = saturate(distance(rootposition.xyz, g_xCamera_CamPos.xyz) / xHairViewDistance);
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
		len *= xLength;

		// expand the particle into a billboard cross section, the patch:
		float3 patchPos = HAIRPATCH[vertexID];
		float2 uv = vertexID < 6 ? patchPos.xy : patchPos.zy;
		uv = uv * float2(0.5f, 0.5f) + 0.5f;
		uv.y = lerp((float)segmentID / (float)xHairSegmentCount, ((float)segmentID + 1) / (float)xHairSegmentCount, uv.y);
		uv.y = 1 - uv.y;
		patchPos.y += 1;

		// Sprite sheet UV transform:
		const uint currentFrame = (xHairFrameStart + rand) % xHairFrameCount;
		uint2 offset = uint2(currentFrame % xHairFramesXY.x, currentFrame / xHairFramesXY.x);
		uv.xy += offset;
		uv.xy *= xHairTexMul;

		// scale the billboard by the texture aspect:
		float2 frame = float2(xHairAspect, 1);
		frame.xy *= len;
		patchPos.xyz *= frame.xyx * 0.5f;

		// simplistic wind effect only affects the top, but leaves the base as is:
		const float waveoffset = dot(rootposition, g_xFrame_WindDirection) * g_xFrame_WindWaveSize + rand / 255.0f * g_xFrame_WindRandomness;
		const float3 wavedir = g_xFrame_WindDirection * (segmentID + patchPos.y);
		const float3 wind = sin(g_xFrame_Time * g_xFrame_WindSpeed + waveoffset) * wavedir;
		const float3 windPrev = sin(g_xFrame_TimePrev * g_xFrame_WindSpeed + waveoffset) * wavedir;

		// rotate the patch into the tangent space of the emitting triangle:
		float3x3 TBN = float3x3(tangent, normal, binormal); // don't derive binormal, because we want the shear!
		patchPos = mul(patchPos, TBN);

		// inset to the emitter a bit, to avoid disconnect:
		float3 position = particleBuffer[particleID].position;
		position.xyz -= normal * 0.1 * len;


		// copy to output:
		Out.pos = float4(position, 1);
		Out.pos.xyz += patchPos;
		float3 savedPos = Out.pos.xyz;
		Out.pos.xyz += wind;
		Out.pos3D = Out.pos.xyz;
		Out.pos = mul(g_xCamera_VP, Out.pos);

		Out.nor = normalize(normal + wind);
		Out.tex = uv;

		Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, float4(savedPos + windPrev, 1));

		Out.color = xColor.rgb;
	}
	else
	{
		Out = (VertexToPixel)0;
	}

	return Out;
}
