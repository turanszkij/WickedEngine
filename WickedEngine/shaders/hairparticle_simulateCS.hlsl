#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

static const half3 HAIRPATCH[] = {
	// root (for every strand):
	half3(-1, -1, 0),
	half3(1, -1, 0),

	// cap (for every segment of every strand):
	half3(-1, 1, 0),
	half3(1, 1, 0),
};

Buffer<uint> meshIndexBuffer : register(t0);
Buffer<float4> meshVertexBuffer_POS : register(t1);
Buffer<half4> meshVertexBuffer_NOR : register(t2);
Buffer<half> meshVertexBuffer_length : register(t3);

RWStructuredBuffer<PatchSimulationData> simulationBuffer : register(u0);
RWBuffer<float4> vertexBuffer_POS : register(u1);
RWBuffer<float4> vertexBuffer_UVS : register(u2);
RWBuffer<uint> culledIndexBuffer : register(u3);
RWStructuredBuffer<IndirectDrawArgsIndexedInstanced> indirectBuffer : register(u4);
RWBuffer<float4> vertexBuffer_POS_RT : register(u5);
RWBuffer<float4> vertexBuffer_NOR : register(u6);
RWBuffer<uint> primitiveBuffer : register(u7);

[numthreads(THREADCOUNT_SIMULATEHAIR, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= xHairStrandCount)
		return;

	const bool regenerate_frame = xHairFlags & HAIR_FLAG_REGENERATE_FRAME;
	const uint gfx_vertexcount_per_strand = (xHairSegmentCount * 2 + 2) * xHairBillboardCount;
	const uint gfx_indexcount_per_strand = 6 * xHairBillboardCount * xHairSegmentCount;
	const uint index0 = DTid.x * gfx_indexcount_per_strand;
	const uint vertexID0 = DTid.x * gfx_vertexcount_per_strand;
	uint v0 = vertexID0;
		
	ShaderGeometry geometry = HairGetGeometry();
	
	RNG rng;
	rng.init(uint2(xHairRandomSeed, DTid.x), 0);
	
	// random triangle on emitter surface:
	const uint triangleCount = xHairBaseMeshIndexCount / 3;
	const uint tri = rng.next_uint(triangleCount);

	// load indices of triangle from index buffer
	uint i0 = meshIndexBuffer[tri * 3 + 0];
	uint i1 = meshIndexBuffer[tri * 3 + 1];
	uint i2 = meshIndexBuffer[tri * 3 + 2];

	// load vertices of triangle from vertex buffer:
	float3 pos0 = meshVertexBuffer_POS[i0].xyz;
	float3 pos1 = meshVertexBuffer_POS[i1].xyz;
	float3 pos2 = meshVertexBuffer_POS[i2].xyz;
	half3 nor0 = meshVertexBuffer_NOR[i0].xyz;
	half3 nor1 = meshVertexBuffer_NOR[i1].xyz;
	half3 nor2 = meshVertexBuffer_NOR[i2].xyz;
	half length0 = meshVertexBuffer_length[i0];
	half length1 = meshVertexBuffer_length[i1];
	half length2 = meshVertexBuffer_length[i2];

	// random barycentric coords:
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
	float3 position = attribute_at_bary(pos0, pos1, pos2, bary);
	position = mul(xHairBaseMeshUnormRemap.GetMatrix(), float4(position, 1)).xyz; // position UNORM -> FLOAT
	half3 target = normalize(attribute_at_bary(nor0, nor1, nor2, bary));
	target = normalize(mul(xHairTransform.GetMatrixAdjoint(), target));
	half3 tangent = normalize(mul(half3(hemispherepoint_cos(rng.next_float(), rng.next_float()).xy, 0), get_tangentspace(target)));
	half3 binormal = cross(target, tangent);
	half strand_length = attribute_at_bary(length0, length1, length2, bary);
	
	// Pick an atlas frame from gradient noise so neighbouring strands vary.
	// With a single rect the modulo is always 0, so skip the noise entirely.
	uint currentFrame = 0;
	if (xHairAtlasRectCount > 1)
	{
		currentFrame = uint(noise_gradient_3D(position * xHairUniformity) * 1000) % xHairAtlasRectCount;
	}
	const HairParticleAtlasRect atlas_rect = xHairAtlasRects[currentFrame];
	
	// Transform particle by the emitter object matrix:
	float3 base = mul(xHairTransform.GetMatrix(), float4(position.xyz, 1)).xyz;
	
	float3 diff = GetCamera().position - base;
	const float distsq = dot(diff, diff);
	const bool distance_culled = distsq > sqr(xHairViewDistance);
	
	// Frustum culling the whole strand at once:
	//	intentionally overestimated, to not disappear as soon in different views (shadow map, etc)
	ShaderSphere sphere;
	sphere.center = base;
	sphere.radius = xHairLength;
	//draw_sphere(sphere.center, sphere.radius);
	const bool visible = !distance_culled && GetCamera().frustum.intersects(sphere);
		
	// Optimization: reduce to 1 atomic operation per wave
	const uint waveAppendCount = WaveActiveCountBits(visible);
	uint waveOffset;
	if (WaveIsFirstLane() && waveAppendCount > 0)
	{
		InterlockedAdd(indirectBuffer[0].IndexCountPerInstance, waveAppendCount * gfx_indexcount_per_strand, waveOffset);
	}
	waveOffset = WaveReadLaneFirst(waveOffset);

	// Append visible indices:
	if (visible)
	{
		uint prevCount = waveOffset + WavePrefixSum(gfx_indexcount_per_strand);
		uint i0 = index0;
		uint ii0 = prevCount;
		for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
		{
			for (uint billboardID = 0; billboardID < xHairBillboardCount; ++billboardID)
			{
				culledIndexBuffer[ii0++] = i0++;
				culledIndexBuffer[ii0++] = i0++;
				culledIndexBuffer[ii0++] = i0++;
				culledIndexBuffer[ii0++] = i0++;
				culledIndexBuffer[ii0++] = i0++;
				culledIndexBuffer[ii0++] = i0++;
			}
		}
	}
	
	half len = lerp(1, rng.next_float(), saturate(xHairRandomness)) * strand_length;
	len *= xHairLength;
	len *= atlas_rect.size;
	len /= (half)xHairSegmentCount;
	const float2 frame = float2(atlas_rect.aspect * xHairAspect * xHairSegmentCount, 1) * len * 0.5;
	const float segment_radius = max(frame.x, frame.y);

	//draw_line(base, base + tangent, float4(1, 0, 0, 1));
	//draw_line(base, base + target, float4(0, 1, 0, 1));
	//draw_line(base, base + binormal, float4(0, 0, 1, 1));

	half3 bend = 0;
	if (xHairFlags & HAIR_FLAG_CAMERA_BEND)
	{
		// Bend down to camera up vector to avoid seeing flat planes from above
		bend = GetCamera().up * (1 - saturate(dot(target, GetCamera().up))) * 0.8;
	}

	// Bottom vertices:
	half3x3 TBN = half3x3(tangent, normalize(target + bend), binormal);
	if (distance_culled)
	{
		// Strand is beyond the view distance, so it is never rendered: no
		// indices are appended for it and its render vertex buffers are never
		// read. Only the raytracing positions need to stay valid, and only when
		// an acceleration structure consumes them. Collapse the strand to its
		// own emitter position (not a shared point): the triangles stay
		// zero-area (so far grass still does not contribute to ray traced
		// lighting), but spreading them across the terrain keeps the raytracing
		// BVH well distributed. Crucially it also means a strand crossing the
		// cull boundary moves only within its own small neighbourhood instead of
		// teleporting to/from a far shared point, so per-frame refits barely
		// disturb the BVH and it does not degrade between rebuilds.
		const uint root_vertexcount = 2 * xHairBillboardCount;
		if (xHairFlags & HAIR_FLAG_RAYTRACED)
		{
			float3 rt_pos = base;
			if (xHairFlags & HAIR_FLAG_UNORM_POS)
			{
				rt_pos = inverse_lerp(geometry.aabb_min, geometry.aabb_max, base); // remap to UNORM
			}
			for (uint i = 0; i < root_vertexcount; ++i)
			{
				vertexBuffer_POS_RT[v0 + i] = float4(rt_pos, 0);
			}
		}
		v0 += root_vertexcount;
	}
	else
	{
		rng.init(uint2(xHairRandomSeed, DTid.x), 1); // reinit random for consistent billboard variation!
		for (uint billboardID = 0; billboardID < xHairBillboardCount; ++billboardID)
		{
			half siz = billboardID == 0 ? 1 : lerp(0.2, 1, rng.next_float());
			half rot = billboardID == 0 ? 0 : (rng.next_float() * PI);
			half3x3 variationMatrix;
			if (billboardID == 0)
			{
				// No size/rotation variation: the variation matrix is the
				// identity, so mul(variation, TBN) reduces to TBN.
				variationMatrix = TBN;
			}
			else
			{
				half2 rot_sincos;
				sincos(rot, rot_sincos.x, rot_sincos.y);
				variationMatrix = mul(half3x3(
					rot_sincos.y * siz, 0, -rot_sincos.x,
					0, siz, 0,
					rot_sincos.x, 0, rot_sincos.y * siz
				), TBN);
			}

			for (uint vertexID = 0; vertexID < 2; ++vertexID)
			{
				half3 patchPos = HAIRPATCH[vertexID];
				float2 uv = patchPos.xy;
				uv = uv * float2(0.5, 0.5) + 0.5;
				uv.y = 1 - uv.y;
				patchPos.y += 1;

				// Sprite sheet UV transform:
				uv.xy = mad(uv.xy, atlas_rect.texMulAdd.xy, atlas_rect.texMulAdd.zw);

				// scale the billboard by the texture aspect:
				patchPos.xyz *= frame.xyx;

				// variation based on billboardID:
				patchPos = mul(patchPos, variationMatrix);

				float3 position = base + patchPos;

				if (xHairFlags & HAIR_FLAG_UNORM_POS)
				{
					position = inverse_lerp(geometry.aabb_min, geometry.aabb_max, position); // remap to UNORM
				}

				vertexBuffer_POS[v0] = float4(position, 0);
				vertexBuffer_NOR[v0] = half4(target, 0);
				vertexBuffer_UVS[v0] = uv.xyxy; // a second uv set could be used here
				if (xHairFlags & HAIR_FLAG_RAYTRACED)
				{
					vertexBuffer_POS_RT[v0] = float4(position, 0);
				}

				v0++;
			}
		}
	}
	
	const half dt = clamp(GetFrame().delta_time, 0, 1.0 / 30.0); // clamp delta time to avoid simulation blowing up

	const half gravityPower = xHairGravityPower;
	const half stiffnessForce = xHairStiffness;
	const half dragForce = xHairDrag;
	const half3 boneAxis = target;
	const half boneLength = len;
	
	for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
	{
		// Identifies the hair strand segment particle:
		const uint particleID = DTid.x * xHairSegmentCount + segmentID;

		// Resolve this segment's tail. Visible strands run the full dynamics;
		// strands beyond the view distance are held at rest pose so the wind
		// sample, integration and collider loop below are skipped. The rest pose
		// is still stored, so the tails stay valid and the strand resumes
		// without a pop when it re-enters the view distance (it is fully
		// dither-faded out before reaching the cull distance, so the transition
		// between simulated and rest pose is never visible).
		float3 tail_current;
		float3 tail_next;
		half3 to_tail;
		if (distance_culled)
		{
			tail_current = base + boneAxis * boneLength;
			tail_next = tail_current;
			to_tail = boneAxis;
		}
		else
		{
			if (regenerate_frame)
			{
				float3 tail = base + boneAxis * boneLength;
				simulationBuffer[particleID].prevTail = tail;
				simulationBuffer[particleID].currentTail = tail;
			}

			tail_current = simulationBuffer[particleID].currentTail;
			float3 tail_prev = simulationBuffer[particleID].prevTail;
			half3 inertia = (tail_current - tail_prev) * (1 - dragForce);
			half3 stiffness = boneAxis * stiffnessForce;
			half3 external = gravityPower * float3(0, -1, 0);
			half3 wind = sample_wind(tail_current, ((float)segmentID + 1) / (float)xHairSegmentCount);
			external += wind;

			tail_next = tail_current + inertia + dt * (stiffness + external);
			to_tail = normalize(tail_next - base);
			tail_next = base + to_tail * boneLength;
		}

		//draw_sphere(tail_next, len);

		// Apply every force and collider:
		for (uint i = forces().first_item(); !distance_culled && (i < forces().end_item()); ++i)
		{
			ShaderEntity entity = load_entity(i);

			[branch]
			if (entity.layerMask & xHairLayerMask)
			{
				const float range = entity.GetRange();
				const uint type = entity.GetType();

				if (type == ENTITY_TYPE_COLLIDER_CAPSULE)
				{
					float3 A = entity.position;
					float3 B = entity.GetColliderTip();
					half3 N = normalize(A - B);
					A -= N * range;
					B += N * range;
					//if (DTid.x == 0)
					//{
					//	draw_sphere(A, range);
					//	draw_sphere(B, range);
					//}
					float3 C = closest_point_on_segment(A, B, tail_next);
					float3 dir = C - tail_next;
					float dist = length(dir);
					dir /= dist;
					dist = dist - range - segment_radius;
					if (dist < 0)
					{
						tail_next += dir * dist;
						to_tail = normalize(tail_next - base);
						tail_next = base + to_tail * boneLength;
					}
				}
				else
				{
					float3 closest_point = closest_point_on_segment(base, tail_next, entity.position);
					float3 dir = entity.position - closest_point;
					float dist = length(dir);
					dir /= dist;

					switch (type)
					{
						case ENTITY_TYPE_FORCEFIELD_POINT:
							tail_next += dt * dir * entity.GetGravity() * (1 - saturate(dist / range));
							to_tail = normalize(tail_next - base);
							tail_next = base + to_tail * boneLength;
							break;
						case ENTITY_TYPE_FORCEFIELD_PLANE:
							tail_next += dt * entity.GetDirection() * entity.GetGravity() * (1 - saturate(dist / range));
							to_tail = normalize(tail_next - base);
							tail_next = base + to_tail * boneLength;
							break;
						case ENTITY_TYPE_COLLIDER_SPHERE:
							dist = dist - range - segment_radius;
							if (dist < 0)
							{
								tail_next += dir * dist;
								to_tail = normalize(tail_next - base);
								tail_next = base + to_tail * boneLength;
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
							dist = dist - segment_radius;
							if (dist < 0)
							{
								float4x4 planeProjection = load_entitymatrix(entity.GetMatrixIndex());
								const float3 clipSpacePos = mul(planeProjection, float4(closest_point, 1)).xyz;
								const float3 uvw = clipspace_to_uvw(clipSpacePos.xyz);
								[branch]
								if (is_saturated(uvw))
								{
									tail_next -= dir * dist;
									to_tail = normalize(tail_next - base);
									tail_next = base + to_tail * boneLength;
								}
							}
							break;
						default:
							break;
					}
				}
			}
		}

		// Don't allow tail to go below the axis plane:
		float below_plane = plane_point_distance(base, boneAxis, tail_next);
		if (below_plane < 0)
		{
			tail_next -= boneAxis * below_plane;
		}

		// Store simulation:
		simulationBuffer[particleID].prevTail = tail_current;
		simulationBuffer[particleID].currentTail = tail_next;

		//draw_point(tail_next, 0.1, float4(0,1,0,1));
		//draw_line(base, tail_next, float4(1,0,0,1));
		
		half3 normal = to_tail;
		
		// Write out render buffers:
		//	These must be persistent, not culled (raytracing, surfels...)
		half3 normal_bend = normalize(normal + bend);
		binormal = cross(normal_bend, tangent);
		tangent = cross(binormal, normal_bend);
		TBN = half3x3(tangent, normal_bend, binormal);
		
		//draw_line(base, base + tangent, float4(1, 0, 0, 1));
		//draw_line(base, base + normal, float4(0, 1, 0, 1));
		//draw_line(base, base + binormal, float4(0, 0, 1, 1));
	
		if (distance_culled)
		{
			// See the root vertices above: collapse these cap vertices to this
			// segment's rest position (spread along the strand), not a shared
			// point, so the raytracing BVH stays well distributed and stable
			// across cull-boundary crossings.
			const uint cap_vertexcount = 2 * xHairBillboardCount;
			float3 pos = base;
			if (xHairFlags & HAIR_FLAG_UNORM_POS)
			{
				pos = inverse_lerp(geometry.aabb_min, geometry.aabb_max, base); // remap to UNORM
			}

			for (uint i = 0; i < cap_vertexcount; ++i)
			{
				vertexBuffer_POS[v0] = float4(pos, 0); // pos must be written always for wetmap
				vertexBuffer_NOR[v0] = half4(normal, 0); // nor must be written always for wetmap

				if (xHairFlags & HAIR_FLAG_RAYTRACED)
				{
					vertexBuffer_POS_RT[v0] = float4(pos, 0);
				}
				v0++;
			}
		}
		else
		{
			rng.init(uint2(xHairRandomSeed, DTid.x), 1); // reinit random for consistent billboard variation!
			for(uint billboardID = 0; billboardID < xHairBillboardCount; ++billboardID)
			{
				half siz = billboardID == 0 ? 1 : lerp(0.2, 1, rng.next_float());
				half rot = billboardID == 0 ? 0 : (rng.next_float() * PI);
				half3x3 variationMatrix;
				if (billboardID == 0)
				{
					// No size/rotation variation: the variation matrix is the
					// identity, so mul(variation, TBN) reduces to TBN.
					variationMatrix = TBN;
				}
				else
				{
					half2 rot_sincos;
					sincos(rot, rot_sincos.x, rot_sincos.y);
					variationMatrix = mul(half3x3(
						rot_sincos.y * siz, 0, -rot_sincos.x,
						0, siz, 0,
						rot_sincos.x, 0, rot_sincos.y * siz
					), TBN);
				}

				for (uint vertexID = 2; vertexID < 4; ++vertexID)
				{
					half3 patchPos = HAIRPATCH[vertexID];
					float2 uv = patchPos.xy;
					uv = uv * float2(0.5, 0.5) + 0.5;
					uv.y = lerp((float)segmentID / (float)xHairSegmentCount, ((float)segmentID + 1) / (float)xHairSegmentCount, uv.y);
					uv.y = 1 - uv.y;
					patchPos.y += 1;

					// Sprite sheet UV transform:
					uv.xy = mad(uv.xy, atlas_rect.texMulAdd.xy, atlas_rect.texMulAdd.zw);

					// scale the billboard by the texture aspect:
					patchPos.xyz *= frame.xyx;

					// variation based on billboardID:
					patchPos = mul(patchPos, variationMatrix);

					float3 position = base + patchPos;

					if (xHairFlags & HAIR_FLAG_UNORM_POS)
					{
						position = inverse_lerp(geometry.aabb_min, geometry.aabb_max, position); // remap to UNORM
					}

					vertexBuffer_POS[v0] = float4(position, 0);
					vertexBuffer_NOR[v0] = half4(normal, 0);
					vertexBuffer_UVS[v0] = uv.xyxy; // a second uv set could be used here
					if (xHairFlags & HAIR_FLAG_RAYTRACED)
					{
						vertexBuffer_POS_RT[v0] = float4(position, 0);
					}

					v0++;
				}
			}
		}

		// Offset next segment root to current tip:
		base = tail_next;
		target = normal;
	}

	// Primitive buffer creation is done here instead of CPU to reduce CPU time spent in buffer creations:
	if (regenerate_frame)
	{
		uint i = index0;
		v0 = vertexID0;
		uint rootOffset = v0;
		uint capOffset = rootOffset + 2 * xHairBillboardCount;
		for (uint billboardID = 0; billboardID < xHairBillboardCount; ++billboardID)
		{
			for (uint segmentID = 0; segmentID < xHairSegmentCount; ++segmentID)
			{
				primitiveBuffer[i++] = rootOffset + 0;
				primitiveBuffer[i++] = rootOffset + 1;
				primitiveBuffer[i++] = capOffset + 0;
				primitiveBuffer[i++] = capOffset + 0;
				primitiveBuffer[i++] = rootOffset + 1;
				primitiveBuffer[i++] = capOffset + 1;
				rootOffset += 2;
				capOffset += 2;
				v0 += 2;
			}
			v0 += 2;
		}
	}
}
