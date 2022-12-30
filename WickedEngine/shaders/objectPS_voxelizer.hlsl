#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_VOXELGI
#define VOXEL_INITIAL_OFFSET 1
//#define ENVMAPRENDERING // modified ambient calc
//#define NO_FLAT_AMBIENT
#include "objectHF.hlsli"
#include "voxelHF.hlsli"
#include "volumetricCloudsHF.hlsli"

// Note: the voxelizer uses an overall simplified material and lighting model (no normal maps, only diffuse light and emissive)

Texture3D<float4> input_previous_radiance : register(t0);

RWTexture3D<uint> output_radiance : register(u0);
RWTexture3D<uint> output_opacity : register(u1);

void VoxelAtomicAverage(inout RWTexture3D<uint> output, in uint3 dest, in float4 color)
{
	float4 addingColor = float4(color.rgb, 1);
	uint newValue = PackVoxelColor(float4(addingColor.rgb, 1.0 / MAX_VOXEL_ALPHA));
	uint expectedValue = 0;
	uint actualValue;

	InterlockedCompareExchange(output[dest], expectedValue, newValue, actualValue);
	while (actualValue != expectedValue)
	{
		expectedValue = actualValue;

		color = UnpackVoxelColor(actualValue);
		color.a *= MAX_VOXEL_ALPHA;

		color.rgb *= color.a;

		color += addingColor;

		color.rgb /= color.a;

		color.a /= MAX_VOXEL_ALPHA;
		newValue = PackVoxelColor(color);

		InterlockedCompareExchange(output[dest], expectedValue, newValue, actualValue);
	}
}

// Note: centroid interpolation is used to avoid floating voxels in some cases
struct PSInput
{
	float4 pos : SV_POSITION;
	centroid float4 color : COLOR;
	float4 uvsets : UVSETS;
	centroid float3 N : NORMAL;
	centroid float3 P : POSITION3D;
};

void main(PSInput input, in uint coverage : SV_Coverage)
{
	float3 N = normalize(input.N);
	float3 P = input.P;
	float coverage_percent = countbits(coverage) / 8.0;

	float4 baseColor = input.color;
	[branch]
	if (GetMaterial().textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		float lod_bias = 0;
		if (GetMaterial().options & SHADERMATERIAL_OPTION_BIT_TRANSPARENT || GetMaterial().alphaTest > 0)
		{
			// If material is non opaque, then we apply bias to avoid sampling such a low
			//	mip level in which alpha is completely gone (helps with trees)
			lod_bias = -10;
		}
		baseColor *= GetMaterial().textures[BASECOLORMAP].SampleBias(sampler_linear_wrap, input.uvsets, lod_bias);
	}
	//baseColor.a = pow(baseColor.a, 64);
	//baseColor.a = 1;
	//baseColor.a *= coverage_percent;

	float4 color = baseColor;
	float3 emissiveColor = GetMaterial().GetEmissive();
	[branch]
	if (any(emissiveColor) && GetMaterial().textures[EMISSIVEMAP].IsValid())
	{
		float4 emissiveMap = GetMaterial().textures[EMISSIVEMAP].Sample(sampler_linear_wrap, input.uvsets);
		emissiveColor *= emissiveMap.rgb * emissiveMap.a;
	}
	//emissiveColor *= coverage_percent;

	Lighting lighting;
	lighting.create(0, 0, 0, 0);

	[branch]
	if (any(xForwardLightMask))
	{
		// Loop through light buckets for the draw call:
		const uint first_item = 0;
		const uint last_item = first_item + GetFrame().lightarray_count - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, 1); // only 2 buckets max (uint2) for forward pass!
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = xForwardLightMask[bucket];

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				ShaderEntity light = load_entity(GetFrame().lightarray_offset + entity_index);

				if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
				{
					continue; // static lights will be skipped (they are used in lightmap baking)
				}

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					float3 L = light.GetDirection();
					const float NdotL = saturate(dot(L, N));

					[branch]
					if (NdotL > 0)
					{
						float3 lightColor = light.GetColor().rgb * NdotL;

						[branch]
						if (light.IsCastingShadow() >= 0)
						{
							[loop]
							for (uint cascade = 0; cascade < GetFrame().shadow_cascade_count; ++cascade)
							{
								const float3 shadow_pos = mul(load_entitymatrix(light.GetMatrixIndex() + cascade), float4(P, 1)).xyz; // ortho matrix, no divide by .w
								const float3 shadow_uv = shadow_pos.xyz * float3(0.5f, -0.5f, 0.5f) + 0.5f;

								if (is_saturated(shadow_uv))
								{
									lightColor *= shadow_2D(light, shadow_pos, shadow_uv.xy, cascade);
									break;
								}
							}

							if (GetFrame().options & OPTION_BIT_VOLUMETRICCLOUDS_SHADOWS)
							{
								lightColor *= shadow_2D_volumetricclouds(P);
							}
						}

						lighting.direct.diffuse += lightColor;
					}
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					float3 L = light.position - P;
					const float dist2 = dot(L, L);
					const float range = light.GetRange();
					const float range2 = range * range;

					[branch]
					if (dist2 < range2)
					{
						const float3 Lunnormalized = L;
						const float dist = sqrt(dist2);
						L /= dist;
						const float NdotL = saturate(dot(L, N));

						[branch]
						if (NdotL > 0)
						{
							float3 lightColor = light.GetColor().rgb * NdotL * attenuation_pointlight(dist, dist2, range, range2);

							[branch]
							if (light.IsCastingShadow() >= 0) {
								lightColor *= shadow_cube(light, Lunnormalized);
							}

							lighting.direct.diffuse += lightColor;
						}
					}
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					float3 L = light.position - P;
					const float dist2 = dot(L, L);
					const float range = light.GetRange();
					const float range2 = range * range;

					[branch]
					if (dist2 < range2)
					{
						const float dist = sqrt(dist2);
						L /= dist;
						const float NdotL = saturate(dot(L, N));

						[branch]
						if (NdotL > 0)
						{
							const float spot_factor = dot(L, light.GetDirection());
							const float spot_cutoff = light.GetConeAngleCos();

							[branch]
							if (spot_factor > spot_cutoff)
							{
								float3 lightColor = light.GetColor().rgb * NdotL * attenuation_spotlight(dist, dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());

								[branch]
								if (light.IsCastingShadow() >= 0)
								{
									float4 ShPos = mul(load_entitymatrix(light.GetMatrixIndex() + 0), float4(P, 1));
									ShPos.xyz /= ShPos.w;
									float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
									[branch]
									if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
									{
										lightColor *= shadow_2D(light, ShPos.xyz, ShTex.xy, 0);
									}
								}

								lighting.direct.diffuse += lightColor;
							}
						}
					}
				}
				break;
				}
			}
		}
	}

	float4 trace = ConeTraceDiffuse(input_previous_radiance, P, N);
	lighting.indirect.diffuse = trace.rgb;
	lighting.indirect.diffuse += GetAmbient(N) * (1 - trace.a);

	color.rgb *= lighting.direct.diffuse / PI + lighting.indirect.diffuse;

	color.rgb += emissiveColor;

	// output:
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[g_xVoxelizer.clipmap_index];
	float3 uvw = GetFrame().vxgi.world_to_clipmap(P, clipmap);
	if (!is_saturated(uvw))
		return;
	uint3 writecoord = floor(uvw * GetFrame().vxgi.resolution);

	float3 aniso_direction = N;

#if 0
	uint face_offset = cubemap_to_uv(aniso_direction).z * GetFrame().vxgi.resolution;
	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offset, 0, 0), color);
	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offset, 0, 0), color.aaaa);
#else
	float3 face_offsets = float3(
		aniso_direction.x > 0 ? 0 : 1,
		aniso_direction.y > 0 ? 2 : 3,
		aniso_direction.z > 0 ? 4 : 5
		) * GetFrame().vxgi.resolution;
	float3 direction_weights = abs(N);
	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.x, 0, 0), color * direction_weights.x);
	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.y, 0, 0), color * direction_weights.y);
	VoxelAtomicAverage(output_radiance, writecoord + uint3(face_offsets.z, 0, 0), color * direction_weights.z);
	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.x, 0, 0), color.aaaa * direction_weights.x);
	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.y, 0, 0), color.aaaa * direction_weights.y);
	VoxelAtomicAverage(output_opacity, writecoord + uint3(face_offsets.z, 0, 0), color.aaaa * direction_weights.z);
#endif

	//bool done = false;
	//while (!done)
	//{
	//	// acquire lock:
	//	uint locked;
	//	InterlockedCompareExchange(lock[writecoord], 0, 1, locked);
	//	if (locked == 0)
	//	{
	//		float4 average = output_albedo[writecoord];
	//		float3 average_normal = output_normal[writecoord];

	//		average.a += 1;
	//		average.rgb += color.rgb;
	//		average_normal.rgb += N * 0.5 + 0.5;

	//		output_albedo[writecoord] = average;
	//		output_normal[writecoord] = average_normal;

	//		InterlockedExchange(lock[writecoord], 0, locked);
	//		done = true;
	//	}
	//}
}
