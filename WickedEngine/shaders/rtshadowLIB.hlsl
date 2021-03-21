#define BRDF_NDOTL_BIAS 0.1
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(EntityTiles, uint, TEXSLOT_RENDERPATH_ENTITYTILES);

static const uint MAX_RTSHADOWS = 16;
RWTEXTURE2D(output, uint4, 0);

Texture2D<float4> bindless_textures[] : register(t0, space1);
ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

struct RayPayload
{
	float color;
};

[shader("raygeneration")]
void RTShadow_Raygen()
{
	const float2 uv = ((float2)DispatchRaysIndex() + 0.5f) / (float2)DispatchRaysDimensions();
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0.0f)
		return;

	const float3 P = reconstructPosition(uv, depth);
	float3 N = normalize(texture_normals.SampleLevel(sampler_point_clamp, uv, 0) * 2 - 1);

	Surface surface;
	surface.init();
	surface.pixel = DispatchRaysIndex().xy;
	surface.P = P;
	surface.N = N;

	const uint2 tileIndex = uint2(floor(surface.pixel * 2 / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	uint4 shadow_mask = 0;
	uint shadow_index = 0;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.Origin = trace_bias_position(P, N);

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
					ShaderEntity light = EntityArray[entity_index];

					if (!light.IsCastingShadow())
					{
						continue;
					}

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					ray.TMax = 0;

					float3 L;

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
						float seed = g_xFrame_FrameCount * 0.001;

						RayPayload payload;
						payload.color = 0;

						for (uint i = 0; i < g_xFrame_RaytracedShadowsSampleCount; ++i)
						{
							float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1; // todo: should be specific to light surface
							ray.Direction = normalize(L + sampling_offset * 0.025);

							TraceRay(
								scene_acceleration_structure,         // AccelerationStructure
								RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,     // RayFlags
								~0,                                   // InstanceInclusionMask
								0,                                    // RayContributionToHitGroupIndex
								0,                                    // MultiplierForGeomtryContributionToShaderIndex
								0,                                    // MissShaderIndex
								ray,                                  // Ray
								payload                               // Payload
							);
						}
						payload.color /= g_xFrame_RaytracedShadowsSampleCount;

						uint mask = uint(saturate(payload.color) * 255); // 8 bits
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

	output[DispatchRaysIndex().xy] = shadow_mask;
}

[shader("closesthit")]
void RTShadow_ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//payload.color = 0;
}

[shader("anyhit")]
void RTShadow_AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	ShaderMesh mesh = bindless_buffers[InstanceID()].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);
	[branch]
	if (material.texture_basecolormap_index < 0)
	{
		AcceptHitAndEndSearch();
		return;
	}
	uint primitiveIndex = PrimitiveIndex();
	uint i0 = bindless_ib[mesh.ib][primitiveIndex * 3 + 0];
	uint i1 = bindless_ib[mesh.ib][primitiveIndex * 3 + 1];
	uint i2 = bindless_ib[mesh.ib][primitiveIndex * 3 + 2];
	float2 uv0 = 0, uv1 = 0, uv2 = 0;
	[branch]
	if (mesh.vb_uv0 >= 0 && material.uvset_baseColorMap == 0)
	{
		uv0 = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i0 * 4));
		uv1 = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i1 * 4));
		uv2 = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i2 * 4));
	}
	else if (mesh.vb_uv1 >= 0 && material.uvset_baseColorMap != 0)
	{
		uv0 = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i0 * 4));
		uv1 = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i1 * 4));
		uv2 = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i2 * 4));
	}
	else
	{
		AcceptHitAndEndSearch();
		return;
	}

	float u = attr.barycentrics.x;
	float v = attr.barycentrics.y;
	float w = 1 - u - v;
	float2 uv = uv0 * w + uv1 * u + uv2 * v;
	float alpha = bindless_textures[material.texture_basecolormap_index].SampleLevel(sampler_point_wrap, uv, 2).a;

	[branch]
	if (alpha - material.alphaTest > 0)
	{
		AcceptHitAndEndSearch();
	}
	else
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RTShadow_Miss(inout RayPayload payload)
{
	payload.color += 1;
}
