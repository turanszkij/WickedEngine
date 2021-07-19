#define BRDF_NDOTL_BIAS 0.1
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

STRUCTUREDBUFFER(EntityTiles, uint, TEXSLOT_RENDERPATH_ENTITYTILES);

static const uint MAX_RTSHADOWS = 16;
RWTEXTURE2D(output, uint4, 0);

#ifdef RTSHADOW
RWTEXTURE2D(output_normals, float3, 1);
RWSTRUCTUREDBUFFER(output_tiles, uint4, 2);
#endif // RTSHADOW

static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = POSTPROCESS_BLOCKSIZE + TILE_BORDER * 2;
groupshared float2 tile_XY[TILE_SIZE * TILE_SIZE];
groupshared float tile_Z[TILE_SIZE * TILE_SIZE];

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
#ifdef RTSHADOW

	// ray traced shadow works better with GBUFFER normal:
	//	Reprojection issues are mostly solved by denoiser anyway
	const float2 uv = ((float2)DTid.xy + 0.5) * xPPResolution_rcp;
	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;
	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	const float3 N = normalize(g1.rgb * 2 - 1);

	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	const float3 P = reconstructPosition(uv, depth);

	uint flatTileIdx = 0;
	if (GTid.y < 4)
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 0), (xPPResolution + uint2(7, 3)) / uint2(8, 4));
	}
	else
	{
		flatTileIdx = flatten2D(Gid.xy * uint2(1, 2) + uint2(0, 1), (xPPResolution + uint2(7, 3)) / uint2(8, 4));
	}
	output_tiles[flatTileIdx] = 0;

	const float2 bluenoise = blue_noise(DTid.xy).xy;

#else

	// Screen space shadow will reconstruct normal from depth:

	const int2 tile_upperleft = Gid.xy * POSTPROCESS_BLOCKSIZE - TILE_BORDER;
	for (uint t = groupIndex; t < TILE_SIZE * TILE_SIZE; t += POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE)
	{
		const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
		const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;
		const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
		const float3 position = reconstructPosition(uv, depth);
		tile_XY[t] = position.xy;
		tile_Z[t] = position.z;
	}
	GroupMemoryBarrierWithGroupSync();

	// reconstruct flat normals from depth buffer:
	//	Explanation: There are two main ways to reconstruct flat normals from depth buffer:
	//		1: use ddx() and ddy() on the reconstructed positions to compute triangle, this has artifacts on depth discontinuities and doesn't work in compute shader
	//		2: Take 3 taps from the depth buffer, reconstruct positions and compute triangle. This can still produce artifacts
	//			on discontinuities, but a little less. To fix the remaining artifacts, we can take 4 taps around the center, and find the "best triangle"
	//			by only computing the positions from those depths that have the least amount of discontinuity

	const uint cross_idx[5] = {
		flatten2D(TILE_BORDER + GTid.xy, TILE_SIZE),				// 0: center
		flatten2D(TILE_BORDER + GTid.xy + int2(1, 0), TILE_SIZE),	// 1: right
		flatten2D(TILE_BORDER + GTid.xy + int2(-1, 0), TILE_SIZE),	// 2: left
		flatten2D(TILE_BORDER + GTid.xy + int2(0, 1), TILE_SIZE),	// 3: down
		flatten2D(TILE_BORDER + GTid.xy + int2(0, -1), TILE_SIZE),	// 4: up
	};

	const float center_Z = tile_Z[cross_idx[0]];

	[branch]
	if (center_Z >= g_xCamera_ZFarP)
		return;

	const uint best_Z_horizontal = abs(tile_Z[cross_idx[1]] - center_Z) < abs(tile_Z[cross_idx[2]] - center_Z) ? 1 : 2;
	const uint best_Z_vertical = abs(tile_Z[cross_idx[3]] - center_Z) < abs(tile_Z[cross_idx[4]] - center_Z) ? 3 : 4;

	float3 p1 = 0, p2 = 0;
	if (best_Z_horizontal == 1 && best_Z_vertical == 4)
	{
		p1 = float3(tile_XY[cross_idx[1]], tile_Z[cross_idx[1]]);
		p2 = float3(tile_XY[cross_idx[4]], tile_Z[cross_idx[4]]);
	}
	else if (best_Z_horizontal == 1 && best_Z_vertical == 3)
	{
		p1 = float3(tile_XY[cross_idx[3]], tile_Z[cross_idx[3]]);
		p2 = float3(tile_XY[cross_idx[1]], tile_Z[cross_idx[1]]);
	}
	else if (best_Z_horizontal == 2 && best_Z_vertical == 4)
	{
		p1 = float3(tile_XY[cross_idx[4]], tile_Z[cross_idx[4]]);
		p2 = float3(tile_XY[cross_idx[2]], tile_Z[cross_idx[2]]);
	}
	else if (best_Z_horizontal == 2 && best_Z_vertical == 3)
	{
		p1 = float3(tile_XY[cross_idx[2]], tile_Z[cross_idx[2]]);
		p2 = float3(tile_XY[cross_idx[3]], tile_Z[cross_idx[3]]);
	}

	const float3 P = float3(tile_XY[cross_idx[0]], tile_Z[cross_idx[0]]);
	const float3 N = normalize(cross(p2 - P, p1 - P));

#endif // RTSHADOW

	Surface surface;
	surface.init();
	surface.pixel = DTid.xy;
	surface.P = P;
	surface.N = N;

	const uint2 tileIndex = uint2(floor(surface.pixel * 2 / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	uint shadow_mask[4] = {0,0,0,0}; // FXC issue: can't dynamically index into uint4, unless unrolling all loops
	uint shadow_index = 0;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.Origin = P;

#ifndef RTSHADOW
	ray.Origin = mul(g_xCamera_View, float4(ray.Origin, 1)).xyz;

	const float range = xPPParams0.x;
	const uint samplecount = xPPParams0.y;
	const float thickness = 0.1;
	const float stepsize = range / samplecount;
	const float offset = abs(dither(DTid.xy + GetTemporalAASampleRotation()));
#endif // RTSHADOW

	[branch]
	if (g_xFrame_LightArrayCount > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = g_xFrame_LightArrayOffset;
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket && shadow_index < MAX_RTSHADOWS; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

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
					shadow_index = entity_index - g_xFrame_LightArrayOffset;
					if (shadow_index >= MAX_RTSHADOWS)
						break;

					ShaderEntity light = EntityArray[entity_index];

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
						L = light.position - surface.P;
						const float dist2 = dot(L, L);
						const float range2 = light.GetRange() * light.GetRange();

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
							if (any(surfaceToLight.NdotL_sss))
							{
								const float SpotFactor = dot(L, light.GetDirection());
								const float spotCutOff = light.GetConeAngleCos();

								[branch]
								if (SpotFactor > spotCutOff)
								{
									ray.TMax = dist;
								}
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

						ray.Direction = normalize(lerp(L, mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), GetTangentSpace(L)), 0.025));

#ifdef RTAPI
						RayQuery<
							RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
						> q;
						q.TraceRayInline(
							scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
							0,								// uint RayFlags
							0xFF,							// uint InstanceInclusionMask
							ray								// RayDesc Ray
						);
						while (q.Proceed())
						{
							ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(q.CandidateInstanceID())].Load<ShaderMesh>(0);
							ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][q.CandidateGeometryIndex()];
							ShaderMaterial material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);
							[branch]
							if (!material.IsCastingShadow())
							{
								continue;
							}
							[branch]
							if (material.texture_basecolormap_index < 0)
							{
								q.CommitNonOpaqueTriangleHit();
								break;
							}

							Surface surface;
							EvaluateObjectSurface(
								mesh,
								subset,
								material,
								q.CandidatePrimitiveIndex(),
								q.CandidateTriangleBarycentrics(),
								q.CandidateObjectToWorld3x4(),
								surface
							);

							[branch]
							if (surface.opacity >= material.alphaTest)
							{
								q.CommitNonOpaqueTriangleHit();
								break;
							}
						}
						shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : 1;
#else
						shadow = TraceRay_Any(newRay, groupIndex) ? 0 : 1;
#endif // RTAPI

#else
						// screen space raymarch shadow:
						ray.Direction = normalize(mul((float3x3)g_xCamera_View, L));
						float3 rayPos = ray.Origin + ray.Direction * stepsize * offset;

						float occlusion = 0;
						[loop]
						for (uint i = 0; i < samplecount; ++i)
						{
							rayPos += ray.Direction * stepsize;

							float4 proj = mul(g_xCamera_Proj, float4(rayPos, 1));
							proj.xyz /= proj.w;
							proj.xy = proj.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

							[branch]
							if (is_saturated(proj.xy))
							{
								const float ray_depth_real = proj.w;
								const float ray_depth_sample = texture_lineardepth.SampleLevel(sampler_point_clamp, proj.xy, 1) * g_xCamera_ZFarP;
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
