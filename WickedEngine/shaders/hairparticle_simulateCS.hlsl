#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

static const float3 HAIRPATCH[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(1, 1, 0),
};

RWSTRUCTUREDBUFFER(simulationBuffer, PatchSimulationData, 0);
RWRAWBUFFER(vertexBuffer_POS, 1);
RWRAWBUFFER(vertexBuffer_TEX, 2);
RWTYPEDBUFFER(primitiveBuffer, uint, 3);
RWTYPEDBUFFER(culledIndexBuffer, uint, 4);
RWRAWBUFFER(counterBuffer, 5);

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND1);
TYPEDBUFFER(meshVertexBuffer_length, float, TEXSLOT_ONDEMAND2);

[numthreads(THREADCOUNT_SIMULATEHAIR, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= xHairParticleCount)
		return;

	// Generate patch:

	// random triangle on emitter surface:
	//	(Note that the usual rand() function is not used because that introduces unnatural clustering with high triangle count)
	uint tri = (uint)((xHairBaseMeshIndexCount / 3) * hash1(DTid.x));

	// load indices of triangle from index buffer
	uint i0 = meshIndexBuffer[tri * 3 + 0];
	uint i1 = meshIndexBuffer[tri * 3 + 1];
	uint i2 = meshIndexBuffer[tri * 3 + 2];

	// load vertices of triangle from vertex buffer:
	float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xHairBaseMeshVertexPositionStride));
	float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xHairBaseMeshVertexPositionStride));
    float3 nor0 = unpack_unitvector(asuint(pos_nor0.w));
    float3 nor1 = unpack_unitvector(asuint(pos_nor1.w));
    float3 nor2 = unpack_unitvector(asuint(pos_nor2.w));
	float length0 = meshVertexBuffer_length[i0];
	float length1 = meshVertexBuffer_length[i1];
	float length2 = meshVertexBuffer_length[i2];

	// random barycentric coords:
	float2 uv = hammersley2d(DTid.x, xHairNumDispatchGroups * THREADCOUNT_SIMULATEHAIR);
	float seed = xHairRandomSeed;
	float f = rand(seed, uv);
	float g = rand(seed, uv);
	[flatten]
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}

	// compute final surface position on triangle from barycentric coords:
	float3 position = pos_nor0.xyz + f * (pos_nor1.xyz - pos_nor0.xyz) + g * (pos_nor2.xyz - pos_nor0.xyz);
	float3 target = normalize(nor0 + f * (nor1 - nor0) + g * (nor2 - nor0));
	float3 tangent = normalize(mul(float3(hemispherepoint_cos(rand(seed, uv), rand(seed, uv)).xy, 0), GetTangentSpace(target)));
	float3 binormal = cross(target, tangent);
	float strand_length = length0 + f * (length1 - length0) + g * (length2 - length0);

	uint tangent_random = 0;
	tangent_random |= (uint)((uint)(tangent.x * 127.5f + 127.5f) << 0);
	tangent_random |= (uint)((uint)(tangent.y * 127.5f + 127.5f) << 8);
	tangent_random |= (uint)((uint)(tangent.z * 127.5f + 127.5f) << 16);
	tangent_random |= (uint)(rand(seed, uv) * 255) << 24;

	uint binormal_length = 0;
	binormal_length |= (uint)((uint)(binormal.x * 127.5f + 127.5f) << 0);
	binormal_length |= (uint)((uint)(binormal.y * 127.5f + 127.5f) << 8);
	binormal_length |= (uint)((uint)(binormal.z * 127.5f + 127.5f) << 16);
	binormal_length |= (uint)(lerp(1, rand(seed, uv), saturate(xHairRandomness)) * strand_length * 255) << 24;

	// Identifies the hair strand root particle:
    const uint strandID = DTid.x * xHairSegmentCount;
    
	// Transform particle by the emitter object matrix:
    float3 base = mul(xHairWorld, float4(position.xyz, 1)).xyz;
    target = normalize(mul((float3x3)xHairWorld, target));
	const float3 root = base;

	float3 normal = 0;
    
	for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
	{
		// Identifies the hair strand segment particle:
		const uint particleID = strandID + segmentID;
		simulationBuffer[particleID].tangent_random = tangent_random;
		simulationBuffer[particleID].binormal_length = binormal_length;

		if (xHairRegenerate)
		{
			simulationBuffer[particleID].position = base;
			simulationBuffer[particleID].normal = target;
			simulationBuffer[particleID].velocity = 0;
		}

        normal += simulationBuffer[particleID].normal;
        normal = normalize(normal);

		float len = (binormal_length >> 24) & 0x000000FF;
		len /= 255.0f;
		len *= xLength;

		float3 tip = base + normal * len;

		// Accumulate forces:
		float3 force = 0;
        for (uint i = 0; i < g_xFrame.ForceFieldArrayCount; ++i)
        {
			ShaderEntity forceField = load_entity(g_xFrame.ForceFieldArrayOffset + i);

			[branch]
			if (forceField.layerMask & xHairLayerMask)
			{
				//float3 dir = forceField.position - PointOnLineSegmentNearestPoint(base, tip, forceField.position);
				float3 dir = forceField.position - tip;
				float dist;
				if (forceField.GetType() == ENTITY_TYPE_FORCEFIELD_POINT) // point-based force field
				{
					//dist = length(dir);
					dist = length(forceField.position - ClosestPointOnSegment(base, tip, forceField.position));
				}
				else // planar force field
				{
					dist = dot(forceField.GetDirection(), dir);
					dir = forceField.GetDirection();
				}

				force += dir * forceField.GetEnergy() * (1 - saturate(dist * forceField.GetRange())); // GetRange() is actually uploaded as 1.0 / range
			}
        }

		// Pull back to rest position:
        force += (target - normal) * xStiffness;

        force *= g_xFrame.DeltaTime;

		// Simulation buffer load:
        float3 velocity = simulationBuffer[particleID].velocity;

		// Apply surface-movement-based velocity:
		const float3 old_base = simulationBuffer[particleID].position;
		const float3 old_normal = simulationBuffer[particleID].normal;
		const float3 old_tip = old_base + old_normal * len;
		const float3 surface_velocity = old_tip - tip;
		velocity += surface_velocity;

		// Apply forces:
		velocity += force;
		normal += velocity * g_xFrame.DeltaTime;

		// Drag:
		velocity *= 0.98f;

		// Store particle:
		simulationBuffer[particleID].position = base;
		simulationBuffer[particleID].normal = normalize(normal);

		// Store simulation data:
		simulationBuffer[particleID].velocity = velocity;


		// Write out render buffers:
		//	These must be persistent, not culled (raytracing, surfels...)
		uint v0 = particleID * 4;
		uint i0 = particleID * 6;
		primitiveBuffer[i0 + 0] = v0 + 0;
		primitiveBuffer[i0 + 1] = v0 + 1;
		primitiveBuffer[i0 + 2] = v0 + 2;
		primitiveBuffer[i0 + 3] = v0 + 2;
		primitiveBuffer[i0 + 4] = v0 + 1;
		primitiveBuffer[i0 + 5] = v0 + 3;

		uint rand = (tangent_random >> 24) & 0x000000FF;
		float3x3 TBN = float3x3(tangent, normal, binormal); // don't derive binormal, because we want the shear!
		float3 rootposition = base - normal * 0.1 * len; // inset to the emitter a bit, to avoid disconnect:
		float2 frame = float2(xHairAspect, 1) * len * 0.5;
		const uint currentFrame = (xHairFrameStart + rand) % xHairFrameCount;
		uint2 offset = uint2(currentFrame % xHairFramesXY.x, currentFrame / xHairFramesXY.x);

		for (uint vertexID = 0; vertexID < 4; ++vertexID)
		{
			// expand the particle into a billboard cross section, the patch:
			float3 patchPos = HAIRPATCH[vertexID];
			float2 uv = vertexID < 6 ? patchPos.xy : patchPos.zy;
			uv = uv * float2(0.5f, 0.5f) + 0.5f;
			uv.y = lerp((float)segmentID / (float)xHairSegmentCount, ((float)segmentID + 1) / (float)xHairSegmentCount, uv.y);
			uv.y = 1 - uv.y;
			patchPos.y += 1;

			// Sprite sheet UV transform:
			uv.xy += offset;
			uv.xy *= xHairTexMul;

			// scale the billboard by the texture aspect:
			patchPos.xyz *= frame.xyx;

			// rotate the patch into the tangent space of the emitting triangle:
			patchPos = mul(patchPos, TBN);

			// simplistic wind effect only affects the top, but leaves the base as is:
			const float waveoffset = dot(rootposition, GetWeather().wind.direction) * GetWeather().wind.wavesize + rand / 255.0f * GetWeather().wind.randomness;
			const float3 wavedir = GetWeather().wind.direction * (segmentID + patchPos.y);
			const float3 wind = sin(g_xFrame.Time * GetWeather().wind.speed + waveoffset) * wavedir;

			float3 position = rootposition + patchPos + wind;

			uint4 data;
			data.xyz = asuint(position);
			data.w = pack_unitvector(normalize(normal + wind));
			vertexBuffer_POS.Store4((v0 + vertexID) * 16, data);
			vertexBuffer_TEX.Store((v0 + vertexID) * 4, pack_half2(uv));
		}

		// Frustum culling:
		uint infrustum = 1;
		float3 center = (base + tip) * 0.5;
		float radius = -len;
		infrustum &= distance(base, g_xCamera.CamPos.xyz) < xHairViewDistance;
		infrustum &= dot(g_xCamera.FrustumPlanes[0], float4(center, 1)) > radius;
		infrustum &= dot(g_xCamera.FrustumPlanes[2], float4(center, 1)) > radius;
		infrustum &= dot(g_xCamera.FrustumPlanes[3], float4(center, 1)) > radius;
		infrustum &= dot(g_xCamera.FrustumPlanes[4], float4(center, 1)) > radius;
		infrustum &= dot(g_xCamera.FrustumPlanes[5], float4(center, 1)) > radius;

		if (infrustum)
		{
			uint prevCount;
			counterBuffer.InterlockedAdd(0, 1, prevCount);
			uint ii0 = prevCount * 6;
			culledIndexBuffer[ii0 + 0] = i0 + 0;
			culledIndexBuffer[ii0 + 1] = i0 + 1;
			culledIndexBuffer[ii0 + 2] = i0 + 2;
			culledIndexBuffer[ii0 + 3] = i0 + 3;
			culledIndexBuffer[ii0 + 4] = i0 + 4;
			culledIndexBuffer[ii0 + 5] = i0 + 5;
		}

		// Offset next segment root to current tip:
		base = tip;
    }
}
