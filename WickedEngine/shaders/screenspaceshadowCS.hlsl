//#define BRDF_NDOTL_BIAS 0.1
#define RAYTRACING_HINT_COHERENT_RAYS // xbox
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

static const uint MAX_RTSHADOWS = 16;
RWTexture2D<uint4> output : register(u0);

#ifdef RTSHADOW
RWTexture2D<float3> output_normals : register(u1);
RWStructuredBuffer<uint4> output_tiles : register(u2);
#endif // RTSHADOW

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = ((float2)DTid.xy + 0.5) * postprocess.resolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0)
		return;

#ifdef RTSHADOW
	uint flatTileIdx = 0;
	if (GTid.y < 4)
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 0), (postprocess.resolution + uint2(7, 3)) / uint2(8, 4));
	}
	else
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 1), (postprocess.resolution + uint2(7, 3)) / uint2(8, 4));
	}
	output_tiles[flatTileIdx] = 0;
#endif // RTSHADOW

	float3 P = reconstruct_position(uv, depth);
	float3 N = decode_oct(texture_normal[DTid.xy * 2]);

	Surface surface;
	surface.init();
	surface.P = P;
	surface.N = N;

	const float4 bluenoise = blue_noise(DTid.xy);

	const uint2 tileIndex = uint2(floor(DTid.xy * 2 / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, GetCamera().entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	uint shadow_mask[4] = {0,0,0,0}; // FXC issue: can't dynamically index into uint4, unless unrolling all loops
	uint shadow_index = 0;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.Origin = P;

#ifndef RTSHADOW
	ray.Origin = mul(GetCamera().view, float4(ray.Origin, 1)).xyz;

	const float range = postprocess.params0.x;
	const uint samplecount = postprocess.params0.y;
	const float thickness = 0.1;
	const float stepsize = range / samplecount;
	const float offset = abs(dither(DTid.xy + GetTemporalAASampleRotation()));
#endif // RTSHADOW

	[branch]
	if (GetFrame().lightarray_count > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = GetFrame().lightarray_offset;
		const uint last_item = first_item + GetFrame().lightarray_count - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket && shadow_index < MAX_RTSHADOWS; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0 && shadow_index < MAX_RTSHADOWS)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					shadow_index = entity_index - GetFrame().lightarray_offset;
					if (shadow_index >= MAX_RTSHADOWS)
						break;

					ShaderEntity light = load_entity(entity_index);

					if (!light.IsCastingShadow())
					{
						continue;
					}

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					float3 L;
					ray.TMax = 0;

					switch (light.GetType())
					{
					default:
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						L = normalize(light.GetDirection());

#ifdef RTSHADOW
						L += mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(L)) * light.GetRadius();
#endif // RTSHADOW

						SurfaceToLight surfaceToLight;
						surfaceToLight.create(surface, L);

						[branch]
						if (any(surfaceToLight.NdotL))
						{
							[branch]
							if (light.IsCastingShadow())
							{
								ray.TMax = FLT_MAX;
							}
						}
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
#ifdef RTSHADOW
						light.position += light.GetDirection() * (bluenoise.z - 0.5) * light.GetLength();
						light.position += mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(normalize(light.position - surface.P))) * light.GetRadius();
#endif // RTSHADOW
						L = light.position - surface.P;
						const float dist2 = dot(L, L);
						const float range = light.GetRange();
						const float range2 = range * range;

						[branch]
						if (dist2 < range2)
						{
							const float3 Lunnormalized = L;
							const float dist = sqrt(dist2);
							L /= dist;

							SurfaceToLight surfaceToLight;
							surfaceToLight.create(surface, L);

							[branch]
							if (any(surfaceToLight.NdotL))
							{
								ray.TMax = dist;
							}
						}
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						float3 Loriginal = normalize(light.position - surface.P);
#ifdef RTSHADOW
						light.position += mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(normalize(light.position - surface.P))) * light.GetRadius();
#endif // RTSHADOW
						L = light.position - surface.P;
						const float dist2 = dot(L, L);
						const float range2 = light.GetRange() * light.GetRange();

						[branch]
						if (dist2 < range2)
						{
							const float dist = sqrt(dist2);
							L /= dist;

							SurfaceToLight surfaceToLight;
							surfaceToLight.create(surface, L);

							[branch]
							if (any(surfaceToLight.NdotL_sss) && (dot(Loriginal, light.GetDirection()) > light.GetConeAngleCos()))
							{
								ray.TMax = dist;
							}
						}
					}
					break;
					}

					[branch]
					if (ray.TMax > 0)
					{
#ifdef RTSHADOW
						// true ray traced shadow:
						uint seed = 0;
						float shadow = 0;

						ray.Direction = L + max3(surface.sss);

#ifdef RTAPI
						wiRayQuery q;
						q.TraceRayInline(
							scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
							RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
							RAY_FLAG_CULL_FRONT_FACING_TRIANGLES |
							RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,								// uint RayFlags
							asuint(postprocess.params1.x),	// uint InstanceInclusionMask
							ray								// RayDesc Ray
						);
						while (q.Proceed())
						{
							if(q.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE) // see xbox coherent ray traversal documentation
								continue;
							PrimitiveID prim;
							prim.primitiveIndex = q.CandidatePrimitiveIndex();
							prim.instanceIndex = q.CandidateInstanceID();
							prim.subsetIndex = q.CandidateGeometryIndex();

							Surface surface;
							surface.init();
							if (!surface.load(prim, q.CandidateTriangleBarycentrics()))
								break;

							float alphatest = clamp(blue_noise(DTid.xy, q.CandidateTriangleRayT()).r, 0, 0.99);

							[branch]
							if (surface.opacity - alphatest >= 0)
							{
								q.CommitNonOpaqueTriangleHit();
							}
						}
						shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : 1;
#else
						shadow = TraceRay_Any(newRay, asuint(postprocess.params1.x), groupIndex) ? 0 : 1;
#endif // RTAPI

#else
						// screen space raymarch shadow:
						ray.Direction = normalize(mul((float3x3)GetCamera().view, L));
						float3 rayPos = ray.Origin + ray.Direction * stepsize * offset;

						float occlusion = 0;
						[loop]
						for (uint i = 0; i < samplecount; ++i)
						{
							rayPos += ray.Direction * stepsize;

							float4 proj = mul(GetCamera().projection, float4(rayPos, 1));
							proj.xyz /= proj.w;
							proj.xy = proj.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

							[branch]
							if (is_saturated(proj.xy))
							{
								const float ray_depth_real = proj.w;
								const float ray_depth_sample = texture_lineardepth.SampleLevel(sampler_point_clamp, proj.xy, 1) * GetCamera().z_far;
								const float ray_depth_delta = ray_depth_real - ray_depth_sample;
								if (ray_depth_delta > 0 && ray_depth_delta < thickness)
								{
									occlusion = 1;

									// screen edge fade:
									float2 fade = max(12 * abs(proj.xy - 0.5) - 5, 0);
									occlusion *= saturate(1 - dot(fade, fade));

									break;
								}
							}
						}
						float shadow = 1 - occlusion;
#endif // RTSHADOW

						uint mask = uint(saturate(shadow) * 255); // 8 bits
						uint mask_shift = (shadow_index % 4) * 8;
						uint mask_bucket = shadow_index / 4;
						shadow_mask[mask_bucket] |= mask << mask_shift;
					}

				}
				else if (entity_index > last_item)
				{
					// force exit:
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}

			}

		}
	}

#ifdef RTSHADOW
	output_normals[DTid.xy] = saturate(N * 0.5 + 0.5);

	// pack 4 lights into tile bitmask:
	int lane_index = (DTid.y % 4) * 8 + (DTid.x % 8);
	for (uint i = 0; i < 4; ++i)
	{
		uint bit = ((shadow_mask[0] >> (i * 8)) & 0xFF) ? (1u << lane_index) : 0;
		InterlockedOr(output_tiles[flatTileIdx][i], bit);
	}
#endif // RTSHADOW

	output[DTid.xy] = uint4(shadow_mask[0], shadow_mask[1], shadow_mask[2], shadow_mask[3]);
}
