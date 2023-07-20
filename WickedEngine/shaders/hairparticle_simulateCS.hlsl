#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

static const float3 HAIRPATCH[] = {
	float3(-1, -1, 0),
	float3(1, -1, 0),
	float3(-1, 1, 0),
	float3(1, 1, 0),
};

Buffer<uint> meshIndexBuffer : register(t0);
ByteAddressBuffer meshVertexBuffer_POS : register(t1);
Buffer<float> meshVertexBuffer_length : register(t2);

RWStructuredBuffer<PatchSimulationData> simulationBuffer : register(u0);
RWByteAddressBuffer vertexBuffer_POS : register(u1);
RWByteAddressBuffer vertexBuffer_UVS : register(u2);
RWBuffer<uint> culledIndexBuffer : register(u3);
RWStructuredBuffer<IndirectDrawArgsIndexedInstanced> indirectBuffer : register(u4);

[numthreads(THREADCOUNT_SIMULATEHAIR, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= xHairParticleCount)
		return;

	// Generate patch:

	// random triangle on emitter surface:
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
	RNG rng;
	rng.init(uint2(xHairRandomSeed, DTid.x), 0);
	float f = rng.next_float();
	float g = rng.next_float();
	[flatten]
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}
	float2 bary = float2(f, g);

	// compute final surface position on triangle from barycentric coords:
	float3 position = attribute_at_bary(pos_nor0.xyz, pos_nor1.xyz, pos_nor2.xyz, bary);
	float3 target = normalize(attribute_at_bary(nor0, nor1, nor2, bary));
	float3 tangent = normalize(mul(float3(hemispherepoint_cos(rng.next_float(), rng.next_float()).xy, 0), get_tangentspace(target)));
	float3 binormal = cross(target, tangent);
	float strand_length = attribute_at_bary(length0, length1, length2, bary);

	uint tangent_random = 0;
	tangent_random |= (uint)((uint)(tangent.x * 127.5f + 127.5f) << 0);
	tangent_random |= (uint)((uint)(tangent.y * 127.5f + 127.5f) << 8);
	tangent_random |= (uint)((uint)(tangent.z * 127.5f + 127.5f) << 16);
	tangent_random |= (uint)(rng.next_float() * 255) << 24;

	uint binormal_length = 0;
	binormal_length |= (uint)((uint)(binormal.x * 127.5f + 127.5f) << 0);
	binormal_length |= (uint)((uint)(binormal.y * 127.5f + 127.5f) << 8);
	binormal_length |= (uint)((uint)(binormal.z * 127.5f + 127.5f) << 16);
	binormal_length |= (uint)(lerp(1, rng.next_float(), saturate(xHairRandomness)) * strand_length * 255) << 24;

	// Identifies the hair strand root particle:
    const uint strandID = DTid.x * xHairSegmentCount;
    
	// Transform particle by the emitter object matrix:
	const float4x4 worldMatrix = xHairTransform.GetMatrix();
    float3 base = mul(worldMatrix, float4(position.xyz, 1)).xyz;
    target = normalize(mul((float3x3)worldMatrix, target));
	const float3 root = base;

	float3 diff = GetCamera().position - root;
	const float distsq = dot(diff, diff);
	const bool distance_culled = distsq > sqr(xHairViewDistance);

	// Bend down to camera up vector to avoid seeing flat planes from above
	const float3 bend = GetCamera().up * (1 - saturate(dot(target, GetCamera().up))) * 0.8;

	float3 normal = 0;

	const float delta_time = clamp(GetFrame().delta_time, 0, 1.0 / 30.0); // clamp delta time to avoid simulation blowing up
    
	for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
	{
		// Identifies the hair strand segment particle:
		const uint particleID = strandID + segmentID;
		simulationBuffer[particleID].tangent_random = tangent_random;
		simulationBuffer[particleID].binormal_length = binormal_length;

		if (xHairRegenerate)
		{
			simulationBuffer[particleID].position = base;
			simulationBuffer[particleID].normal_velocity = f32tof16(target);
		}

        normal += f16tof32(simulationBuffer[particleID].normal_velocity);
        normal = normalize(normal);

		float len = (binormal_length >> 24) & 0x000000FF;
		len /= 255.0f;
		len *= xLength;

		float3 tip = base + normal * len;
		float3 midpoint = lerp(base, tip, 0.5);

		// Accumulate forces, apply colliders:
		float3 force = 0;
        for (uint i = 0; i < GetFrame().forcefieldarray_count; ++i)
        {
			ShaderEntity entity = load_entity(GetFrame().forcefieldarray_offset + i);

			[branch]
			if (entity.layerMask & xHairLayerMask)
			{
				const float range = entity.GetRange();
				const uint type = entity.GetType();

				if (type == ENTITY_TYPE_COLLIDER_CAPSULE)
				{
					float3 A = entity.position;
					float3 B = entity.GetColliderTip();
					float3 N = normalize(A - B);
					A -= N * range;
					B += N * range;
					float3 C = closest_point_on_segment(A, B, midpoint);
					float3 dir = C - midpoint;
					float dist = length(dir);
					dir /= dist;
					dist = dist - range - len * 0.5;
					if (dist < 0)
					{
						tip = tip - dir * dist;
					}
				}
				else
				{
					float3 closest_point = closest_point_on_segment(base, tip, entity.position);
					float3 dir = entity.position - closest_point;
					float dist = length(dir);
					dir /= dist;

					switch (type)
					{
					case ENTITY_TYPE_FORCEFIELD_POINT:
						force += dir * entity.GetGravity() * (1 - saturate(dist / range));
						break;
					case ENTITY_TYPE_FORCEFIELD_PLANE:
						force += entity.GetDirection() * entity.GetGravity() * (1 - saturate(dist / range));
						break;
					case ENTITY_TYPE_COLLIDER_SPHERE:
						dist = dist - range - len;
						if (dist < 0)
						{
							tip = tip - dir * dist;
						}
						break;
					case ENTITY_TYPE_COLLIDER_PLANE:
						dir = normalize(entity.GetDirection());
						dist = plane_point_distance(entity.position, dir, closest_point);
						if (dist < 0)
						{
							dir *= -1;
							dist = abs(dist);
						}
						dist = dist - len;
						if (dist < 0)
						{
							float4x4 planeProjection = load_entitymatrix(entity.GetMatrixIndex());
							const float3 clipSpacePos = mul(planeProjection, float4(closest_point, 1)).xyz;
							const float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
							[branch]
							if (is_saturated(uvw))
							{
								tip = tip + dir * dist;
							}
						}
						break;
					default:
						break;
					}
				}
			}
        }

		// Pull back to rest position:
        force += (target - normal) * xStiffness;

        force *= delta_time;

		// Simulation buffer load:
        float3 velocity = f16tof32(simulationBuffer[particleID].normal_velocity >> 16u);

		// Apply surface-movement-based velocity:
		const float3 old_base = simulationBuffer[particleID].position;
		const float3 old_normal = f16tof32(simulationBuffer[particleID].normal_velocity);
		const float3 old_tip = old_base + old_normal * len;
		const float3 surface_velocity = old_tip - tip;
		velocity += surface_velocity;

		// Apply forces:
		float3 newVelocity = velocity + force;
		float3 newNormal = normal + newVelocity * delta_time;
		newNormal = normalize(newNormal);
		if (dot(target, newNormal) > 0.5) // clamp the offset
		{
			normal = newNormal;
			velocity = newVelocity;
		}

		// Drag:
		velocity *= 0.98f;

		// Store simulation data:
		simulationBuffer[particleID].position = base;
		simulationBuffer[particleID].normal_velocity = f32tof16(normal) | (f32tof16(velocity) << 16u);

		// Write out render buffers:
		//	These must be persistent, not culled (raytracing, surfels...)
		uint v0 = particleID * 4;
		uint i0 = particleID * 6;

		uint rand = (tangent_random >> 24) & 0x000000FF;
		float3x3 TBN = float3x3(tangent, normalize(normal + bend), binormal); // don't derive binormal, because we want the shear!
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
			const float3 wind = compute_wind(rootposition, segmentID + patchPos.y);

			float3 position = rootposition + patchPos + wind;
			if (distance_culled)
			{
				position = 0;
			}

			uint4 data;
			data.xyz = asuint(position);
			data.w = pack_unitvector(normalize(normal + wind));
			vertexBuffer_POS.Store4((v0 + vertexID) * 16, data);
			vertexBuffer_UVS.Store2((v0 + vertexID) * 8, pack_half4(float4(uv, uv))); // a second uv set could be used here
		}

		// Frustum culling:
		ShaderSphere sphere;
		sphere.center = (base + tip) * 0.5;
		sphere.radius = len;

		const bool visible = !distance_culled && GetCamera().frustum.intersects(sphere);

		// Optimization: reduce to 1 atomic operation per wave
		const uint waveAppendCount = WaveActiveCountBits(visible);
		uint waveOffset;
		if (WaveIsFirstLane() && waveAppendCount > 0)
		{
			InterlockedAdd(indirectBuffer[0].IndexCountPerInstance, waveAppendCount * 6, waveOffset);
		}
		waveOffset = WaveReadLaneFirst(waveOffset) / 6;

		if (visible)
		{
			uint prevCount = waveOffset + WavePrefixSum(1);
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
