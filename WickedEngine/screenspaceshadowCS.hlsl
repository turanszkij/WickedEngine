#define BRDF_NDOTL_BIAS 0.1
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(EntityTiles, uint, TEXSLOT_RENDERPATH_ENTITYTILES);

static const uint MAX_RTSHADOWS = 16;
RWTEXTURE2D(output, uint4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const float2 uv = ((float2)DTid.xy + 0.5f) * xPPResolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1);
	if (depth == 0.0f)
		return;

	const float3 P = reconstructPosition(uv, depth);
	float3 N = normalize(texture_normals.SampleLevel(sampler_point_clamp, uv, 0) * 2 - 1);

	Surface surface;
	surface.init();
	surface.pixel = DTid.xy;
	surface.P = P + N * 0.05;
	surface.N = N;

	const uint2 tileIndex = uint2(floor(surface.pixel * 2 / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	uint shadow_mask[4] = {0,0,0,0};
	uint shadow_index = 0;

	const float3 rayOrigin = mul(g_xCamera_View, float4(surface.P, 1)).xyz;

	const float range = xPPParams0.x;
	const uint samplecount = xPPParams0.y;
	const float thickness = 0.1;
	const float stepsize = range / samplecount;
	const float offset = abs(dither(DTid.xy + GetTemporalAASampleRotation()));

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
				bucket_bits ^= 1 << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
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
					bool has_shadow = false;

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
								has_shadow = true;
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
								has_shadow = true;
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
									has_shadow = true;
								}
							}
						}
					}
					break;
					}

					[branch]
					if (has_shadow)
					{
						const float3 rayDirection = normalize(mul((float3x3)g_xCamera_View, L));
						float3 rayPos = rayOrigin + rayDirection * stepsize * offset;

						float occlusion = 0;

						[loop]
						for (uint i = 0; i < samplecount; ++i)
						{
							rayPos += rayDirection * stepsize;

							float4 proj = mul(g_xCamera_Proj, float4(rayPos, 1));
							proj.xyz /= proj.w;
							proj.xy = proj.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

							[branch]
							if (is_saturated(proj.xy))
							{
								const float ray_depth_real = proj.w;
								const float ray_depth_sample = texture_lineardepth.SampleLevel(sampler_point_clamp, proj.xy, 0) * g_xCamera_ZFarP;
								const float ray_depth_delta = ray_depth_real - ray_depth_sample;
								if (ray_depth_delta > 0 && ray_depth_delta < thickness)
								{
									occlusion = 1;

									// screen edge fade:
									float2 fade = max(12 * abs(uv - 0.5) - 5, 0);
									occlusion *= saturate(1 - dot(fade, fade));

									break;
								}
							}
						}
						float shadow = 1 - occlusion;

						uint mask = uint(saturate(shadow) * 255); // 8 bits
						uint mask_shift = (shadow_index % 4) * 8;
						uint mask_bucket = shadow_index / 4;
						shadow_mask[mask_bucket] |= mask << mask_shift;
					}

					// (**) cannot detect exactly same contribution as is pixel shaders!
					//	So we always increment it for shadowed light, even if the
					//	shadow contribution is not traced
					//
					//	This is because in the pixel shader, we will detect the shadow
					//	contribution more precisely due to more precise surface normals
					shadow_index++;

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

	output[DTid.xy] = uint4(shadow_mask[0], shadow_mask[1], shadow_mask[2], shadow_mask[3]);
}
