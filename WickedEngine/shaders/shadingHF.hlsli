#ifndef WI_SHADING_HF
#define WI_SHADING_HF
#include "globals.hlsli"
#include "surfaceHF.hlsli"
#include "lightingHF.hlsli"
#include "brdf.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "ShaderInterop_DDGI.h"

inline void LightMapping(in int lightmap, in float2 ATLAS, inout Lighting lighting, inout Surface surface)
{
	[branch]
	if (lightmap >= 0)
	{
		Texture2D<float4> texture_lightmap = bindless_textures[NonUniformResourceIndex(lightmap)];
#ifdef LIGHTMAP_QUALITY_BICUBIC
		lighting.indirect.diffuse = (half3)SampleTextureCatmullRom(texture_lightmap, sampler_linear_clamp, ATLAS).rgb;
#else
		lighting.indirect.diffuse = (half3)texture_lightmap.SampleLevel(sampler_linear_clamp, ATLAS, 0).rgb;
#endif // LIGHTMAP_QUALITY_BICUBIC

		surface.SetGIApplied(true);
	}
}

inline half3 PlanarReflection(in Surface surface, in half2 bumpColor)
{
	[branch]
	if (GetCamera().texture_reflection_index >= 0)
	{
		float4 reflectionUV = mul(GetCamera().reflection_view_projection, float4(surface.P, 1));
		reflectionUV.xy /= reflectionUV.w;
		reflectionUV.xy = clipspace_to_uv(reflectionUV.xy);
		return (half3)bindless_textures[GetCamera().texture_reflection_index].SampleLevel(sampler_linear_clamp, reflectionUV.xy + bumpColor, 0).rgb;
	}
	return 0;
}

inline void ForwardLighting(inout Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	half4 envmapAccumulation = 0;

#ifndef ENVMAPRENDERING
#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (xForwardEnvProbeMask != 0)
	{
		uint bucket_bits = xForwardEnvProbeMask;

		[loop]
		while (WaveActiveAnyTrue(bucket_bits != 0 && envmapAccumulation.a < 0.99))
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket_bit_index;
			bucket_bits ^= 1u << bucket_bit_index;

			ShaderEntity probe = load_entity(GetFrame().envprobearray_offset + entity_index);
				
			float4x4 probeProjection = load_entitymatrix(probe.GetMatrixIndex());
			const int probeTexture = asint(probeProjection[3][0]);
			probeProjection[3] = float4(0, 0, 0, 1);
			TextureCube cubemap = bindless_cubemaps[probeTexture];
			
			// under here will be VGPR!
			if ((probe.layerMask & surface.layerMask) == 0)
				continue;
			const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
			const float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
			[branch]
			if (is_saturated(uvw))
			{
				const half4 envmapColor = (half4)EnvironmentReflection_Local(cubemap, surface, probe, probeProjection, clipSpacePos);
				// perform manual blending of probes:
				//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
				envmapAccumulation.rgb = mad(1 - envmapAccumulation.a, envmapColor.a * envmapColor.rgb, envmapAccumulation.rgb);
				envmapAccumulation.a = mad(1 - envmapColor.a, envmapAccumulation.a, envmapColor.a);
				[branch]
				if (envmapAccumulation.a >= 1.0)
				{
					// force exit:
					break;
				}
			}

		}
	}
#endif // DISABLE_LOCALENVPMAPS
#endif //ENVMAPRENDERING

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

#ifndef DISABLE_VOXELGI
	VoxelGI(surface, lighting);
#endif //DISABLE_VOXELGI

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
				
				// under here will be VGPR!
				if ((light.layerMask & surface.layerMask) == 0)
					continue;
				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					light_directional(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					light_point(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					light_spot(light, surface, lighting);
				}
				break;
				}
			}
		}
	}

	[branch]
	if (!surface.IsGIApplied() && GetScene().ddgi.color_texture >= 0)
	{
		lighting.indirect.diffuse = ddgi_sample_irradiance(surface.P, surface.N);
		surface.SetGIApplied(true);
	}

}

inline void ForwardDecals(inout Surface surface, inout half4 surfaceMap, SamplerState sam)
{
#ifndef DISABLE_DECALS
	[branch]
	if (xForwardDecalMask == 0)
		return;

	// decals are enabled, loop through them first:
	half4 decalAccumulation = 0;
	half4 decalBumpAccumulation = 0;
	half4 decalSurfaceAccumulation = 0;
	half decalSurfaceAccumulationAlpha = 0;
	const float3 P_dx = ddx_coarse(surface.P);
	const float3 P_dy = ddy_coarse(surface.P);

	uint bucket_bits = xForwardDecalMask;

	[loop]
	while (WaveActiveAnyTrue(bucket_bits != 0 && decalAccumulation.a < 1 && decalBumpAccumulation.a < 1 && decalSurfaceAccumulationAlpha < 1))
	{
		// Retrieve global entity index from local bucket, then remove bit from local bucket:
		const uint bucket_bit_index = firstbitlow(bucket_bits);
		const uint entity_index = bucket_bit_index;
		bucket_bits ^= 1u << bucket_bit_index;

		ShaderEntity decal = load_entity(GetFrame().decalarray_offset + entity_index);

		float4x4 decalProjection = load_entitymatrix(decal.GetMatrixIndex());
		const int decalTexture = asint(decalProjection[3][0]);
		const int decalNormal = asint(decalProjection[3][1]);
		const int decalSurfacemap = asint(decalProjection[3][2]);
		const int decalDisplacementmap = asint(decalProjection[3][3]);
		decalProjection[3] = float4(0, 0, 0, 1);

		// under here will be VGPR!
		if ((decal.layerMask & surface.layerMask) == 0)
			continue;
		const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
		float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
		[branch]
		if (is_saturated(uvw))
		{
			uvw.xy = mad(uvw.xy, decal.shadowAtlasMulAdd.xy, decal.shadowAtlasMulAdd.zw);
			// mipmapping needs to be performed by hand:
			const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy;
			const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy;
			half4 decalColor = decal.GetColor();
			// blend out if close to cube Z:
			const half edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
			const half slopeBlend = decal.GetConeAngleCos() > 0 ? pow(saturate(dot(surface.N, decal.GetDirection())), decal.GetConeAngleCos()) : 1;
			decalColor.a *= edgeBlend * slopeBlend;
			[branch]
			if (decalDisplacementmap >= 0)
			{
				const half3 t = (half3)get_right(decalProjection);
				const half3 b = -(half3)get_up(decalProjection);
				const half3 n = (half3)surface.N;
				const half3x3 tbn = half3x3(t, b, n);
				float4 inoutuv = uvw.xyxy;
				ParallaxOcclusionMapping_Impl(
					inoutuv,
					surface.V,
					tbn,
					decal.GetLength(),
					bindless_textures[decalDisplacementmap],
					uvw.xy,
					decalDX,
					decalDY,
					sampler_linear_clamp
				);
				uvw.xy = saturate(inoutuv.xy);
			}
			[branch]
			if (decalTexture >= 0)
			{
				decalColor *= (half4)bindless_textures[decalTexture].SampleGrad(sam, uvw.xy, decalDX, decalDY);
				if ((decal.GetFlags() & ENTITY_FLAG_DECAL_BASECOLOR_ONLY_ALPHA) == 0)
				{
					// perform manual blending of decals:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
					decalAccumulation.rgb = mad(1 - decalAccumulation.a, decalColor.a * decalColor.rgb, decalAccumulation.rgb);
					decalAccumulation.a = mad(1 - decalColor.a, decalAccumulation.a, decalColor.a);
				}
			}
			[branch]
			if (decalNormal >= 0)
			{
				half3 decalBumpColor = half3(bindless_textures[decalNormal].SampleGrad(sam, uvw.xy, decalDX, decalDY).rg, 1);
				decalBumpColor = decalBumpColor * 2 - 1;
				decalBumpColor.rg *= decal.GetAngleScale();
				decalBumpAccumulation.rgb = mad(1 - decalBumpAccumulation.a, decalColor.a * decalBumpColor.rgb, decalBumpAccumulation.rgb);
				decalBumpAccumulation.a = mad(1 - decalColor.a, decalBumpAccumulation.a, decalColor.a);
			}
			[branch]
			if (decalSurfacemap >= 0)
			{
				half4 decalSurfaceColor = (half4)bindless_textures[decalSurfacemap].SampleGrad(sam, uvw.xy, decalDX, decalDY);
				decalSurfaceAccumulation = mad(1 - decalSurfaceAccumulationAlpha, decalColor.a * decalSurfaceColor, decalSurfaceAccumulation);
				decalSurfaceAccumulationAlpha = mad(1 - decalColor.a, decalSurfaceAccumulationAlpha, decalColor.a);
			}
		}
	}

	surface.baseColor.rgb = mad(surface.baseColor.rgb, 1 - decalAccumulation.a, decalAccumulation.rgb);
	surface.bumpColor.rgb = mad(surface.bumpColor.rgb, 1 - decalBumpAccumulation.a, decalBumpAccumulation.rgb);
	surfaceMap = mad(surfaceMap, 1 - decalSurfaceAccumulationAlpha, decalSurfaceAccumulation);
#endif // DISABLE_DECALS
}

inline uint GetFlatTileIndex(uint2 pixel)
{
	const uint2 tileIndex = uint2(floor(pixel / TILED_CULLING_BLOCKSIZE));
	return flatten2D(tileIndex, GetCamera().entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;
}

inline void TiledLighting(inout Surface surface, inout Lighting lighting, uint flatTileIndex)
{
#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	half4 envmapAccumulation = 0;

#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (GetFrame().envprobearray_count > 0)
	{
		// Loop through envmap buckets in the tile:
		const uint first_item = GetFrame().envprobearray_offset;
		const uint last_item = first_item + GetFrame().envprobearray_count - 1;
		const uint first_bucket = first_item / 32u;
		const uint last_bucket = min(last_item / 32u, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

#ifndef ENTITY_TILE_UNIFORM
			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));
#endif // ENTITY_TILE_UNIFORM

			[loop]
			while (WaveActiveAnyTrue(bucket_bits != 0 && envmapAccumulation.a < 0.99))
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					ShaderEntity probe = load_entity(entity_index);

					float4x4 probeProjection = load_entitymatrix(probe.GetMatrixIndex());
					const int probeTexture = asint(probeProjection[3][0]);
					probeProjection[3] = float4(0, 0, 0, 1);
					TextureCube cubemap = bindless_cubemaps[probeTexture];
					
					// under here will be VGPR!
					if ((probe.layerMask & surface.layerMask) == 0)
						continue;
					const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
					const float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
					[branch]
					if (is_saturated(uvw))
					{
						const half4 envmapColor = (half4)EnvironmentReflection_Local(cubemap, surface, probe, probeProjection, clipSpacePos);
						// perform manual blending of probes:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
						envmapAccumulation.rgb = mad(1 - envmapAccumulation.a, envmapColor.a * envmapColor.rgb, envmapAccumulation.rgb);
						envmapAccumulation.a = mad(1 - envmapColor.a, envmapAccumulation.a, envmapColor.a);
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
#endif // DISABLE_LOCALENVPMAPS

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

#ifndef DISABLE_VOXELGI
#ifdef TRANSPARENT // opaque tiled forward: voxel GI was rendered in separate pass
	VoxelGI(surface, lighting);
#endif // TRANSPARENT
#endif //DISABLE_VOXELGI

	[branch]
	if (GetFrame().lightarray_count > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = GetFrame().lightarray_offset;
		const uint last_item = first_item + GetFrame().lightarray_count - 1;
		const uint first_bucket = first_item / 32u;
		const uint last_bucket = min(last_item / 32u, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = load_entitytile(flatTileIndex + bucket);

#ifndef ENTITY_TILE_UNIFORM
			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));
#endif // ENTITY_TILE_UNIFORM

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1u << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					ShaderEntity light = load_entity(entity_index);

					half shadow_mask = 1;
#if defined(SHADOW_MASK_ENABLED) && !defined(TRANSPARENT)
					[branch]
					if (light.IsCastingShadow() && (GetFrame().options & OPTION_BIT_SHADOW_MASK) && (GetCamera().options & SHADERCAMERA_OPTION_USE_SHADOW_MASK) && GetCamera().texture_rtshadow_index >= 0)
					{
						uint shadow_index = entity_index - GetFrame().lightarray_offset;
						if (shadow_index < 16)
						{
							shadow_mask = (half)bindless_textures2DArray[GetCamera().texture_rtshadow_index][uint3(surface.pixel, shadow_index)].r;
						}
					}
#endif // SHADOW_MASK_ENABLED && !TRANSPARENT

					// under here will be VGPR!
					if ((light.layerMask & surface.layerMask) == 0)
						continue;
					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						light_directional(light, surface, lighting, shadow_mask);
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						light_point(light, surface, lighting, shadow_mask);
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						light_spot(light, surface, lighting, shadow_mask);
					}
					break;
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

#ifndef TRANSPARENT
	[branch]
	if (!surface.IsGIApplied() && GetCamera().texture_rtdiffuse_index >= 0)
	{
		lighting.indirect.diffuse = (half3)bindless_textures[GetCamera().texture_rtdiffuse_index][surface.pixel].rgb;
		surface.SetGIApplied(true);
	}

	[branch]
	if (!surface.IsGIApplied() && GetFrame().options & OPTION_BIT_SURFELGI_ENABLED && GetCamera().texture_surfelgi_index >= 0 && surfel_cellvalid(surfel_cell(surface.P)))
	{
		lighting.indirect.diffuse = (half3)bindless_textures[GetCamera().texture_surfelgi_index][surface.pixel].rgb;
		surface.SetGIApplied(true);
	}

	[branch]
	if (!surface.IsGIApplied() && GetCamera().texture_vxgi_diffuse_index >= 0)
	{
		lighting.indirect.diffuse = (half3)bindless_textures[GetCamera().texture_vxgi_diffuse_index][surface.pixel].rgb;
		surface.SetGIApplied(true);
	}
	[branch]
	if (GetCamera().texture_vxgi_specular_index >= 0)
	{
		half4 vxgi_specular = (half4)bindless_textures[GetCamera().texture_vxgi_specular_index][surface.pixel];
		lighting.indirect.specular = vxgi_specular.rgb * surface.F + lighting.indirect.specular * (1 - vxgi_specular.a);
	}
#endif // TRANSPARENT

	[branch]
	if (!surface.IsGIApplied() && GetScene().ddgi.color_texture >= 0)
	{
		lighting.indirect.diffuse = ddgi_sample_irradiance(surface.P, surface.N);
		surface.SetGIApplied(true);
	}

}

inline void TiledDecals(inout Surface surface, uint flatTileIndex, inout half4 surfaceMap, SamplerState sam)
{
#ifndef DISABLE_DECALS
	[branch]
	if (GetFrame().decalarray_count == 0)
		return;

	// decals are enabled, loop through them first:
	half4 decalAccumulation = 0;
	half4 decalBumpAccumulation = 0;
	half4 decalSurfaceAccumulation = 0;
	half decalSurfaceAccumulationAlpha = 0;

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
	const float3 P_dx = surface.P_dx;
	const float3 P_dy = surface.P_dy;
#else
	const float3 P_dx = ddx_coarse(surface.P);
	const float3 P_dy = ddy_coarse(surface.P);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

	// Loop through decal buckets in the tile:
	const uint first_item = GetFrame().decalarray_offset;
	const uint last_item = first_item + GetFrame().decalarray_count - 1;
	const uint first_bucket = first_item / 32;
	const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
	[loop]
	for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
	{
		uint bucket_bits = load_entitytile(flatTileIndex + bucket);

#ifndef ENTITY_TILE_UNIFORM
		// This is the wave scalarizer from Improved Culling - Siggraph 2017 [Drobot]:
		bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));
#endif // ENTITY_TILE_UNIFORM

		[loop]
		while (WaveActiveAnyTrue(bucket_bits != 0 && decalAccumulation.a < 1 && decalBumpAccumulation.a < 1 && decalSurfaceAccumulationAlpha < 1))
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket * 32 + bucket_bit_index;
			bucket_bits ^= 1u << bucket_bit_index;

			[branch]
			if (entity_index >= first_item && entity_index <= last_item)
			{
				ShaderEntity decal = load_entity(entity_index);

				float4x4 decalProjection = load_entitymatrix(decal.GetMatrixIndex());
				const int decalTexture = asint(decalProjection[3][0]);
				const int decalNormal = asint(decalProjection[3][1]);
				const int decalSurfacemap = asint(decalProjection[3][2]);
				const int decalDisplacementmap = asint(decalProjection[3][3]);
				decalProjection[3] = float4(0, 0, 0, 1);
				
				// under here will be VGPR!
				if ((decal.layerMask & surface.layerMask) == 0)
					continue;
				const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
				float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
				[branch]
				if (is_saturated(uvw))
				{
					uvw.xy = mad(uvw.xy, decal.shadowAtlasMulAdd.xy, decal.shadowAtlasMulAdd.zw);
					// mipmapping needs to be performed by hand:
					const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy;
					const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy;
					half4 decalColor = decal.GetColor();
					// blend out if close to cube Z:
					const half edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
					const half slopeBlend = decal.GetConeAngleCos() > 0 ? pow(saturate(dot(surface.N, decal.GetDirection())), decal.GetConeAngleCos()) : 1;
					decalColor.a *= edgeBlend * slopeBlend;
					[branch]
					if (decalDisplacementmap >= 0)
					{
						const half3 t = (half3)get_right(decalProjection);
						const half3 b = -(half3)get_up(decalProjection);
						const half3 n = (half3)surface.N;
						const half3x3 tbn = half3x3(t, b, n);
						float4 inoutuv = uvw.xyxy;
						ParallaxOcclusionMapping_Impl(
							inoutuv,
							surface.V,
							tbn,
							decal.GetLength(),
							bindless_textures[decalDisplacementmap],
							uvw.xy,
							decalDX,
							decalDY,
							sampler_linear_clamp
						);
						uvw.xy = saturate(inoutuv.xy);
					}
					[branch]
					if (decalTexture >= 0)
					{
						decalColor *= (half4)bindless_textures[decalTexture].SampleGrad(sam, uvw.xy, decalDX, decalDY);
						if ((decal.GetFlags() & ENTITY_FLAG_DECAL_BASECOLOR_ONLY_ALPHA) == 0)
						{
							// perform manual blending of decals:
							//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
							decalAccumulation.rgb = mad(1 - decalAccumulation.a, decalColor.a * decalColor.rgb, decalAccumulation.rgb);
							decalAccumulation.a = mad(1 - decalColor.a, decalAccumulation.a, decalColor.a);
						}
					}
					[branch]
					if (decalNormal >= 0)
					{
						half3 decalBumpColor = half3(bindless_textures[decalNormal].SampleGrad(sam, uvw.xy, decalDX, decalDY).rg, 1);
						decalBumpColor = decalBumpColor * 2 - 1;
						decalBumpColor.rg *= decal.GetAngleScale();
						decalBumpAccumulation.rgb = mad(1 - decalBumpAccumulation.a, decalColor.a * decalBumpColor.rgb, decalBumpAccumulation.rgb);
						decalBumpAccumulation.a = mad(1 - decalColor.a, decalBumpAccumulation.a, decalColor.a);
					}
					[branch]
					if (decalSurfacemap >= 0)
					{
						half4 decalSurfaceColor = (half4)bindless_textures[decalSurfacemap].SampleGrad(sam, uvw.xy, decalDX, decalDY);
						decalSurfaceAccumulation = mad(1 - decalSurfaceAccumulationAlpha, decalColor.a * decalSurfaceColor, decalSurfaceAccumulation);
						decalSurfaceAccumulationAlpha = mad(1 - decalColor.a, decalSurfaceAccumulationAlpha, decalColor.a);
					}
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

	surface.baseColor.rgb = mad(surface.baseColor.rgb, 1 - decalAccumulation.a, decalAccumulation.rgb);
	surface.bumpColor.rgb = mad(surface.bumpColor.rgb, 1 - decalBumpAccumulation.a, decalBumpAccumulation.rgb);
	surfaceMap = mad(surfaceMap, 1 - decalSurfaceAccumulationAlpha, decalSurfaceAccumulation);
#endif // DISABLE_DECALS
}

inline void ApplyFog(in float distance, float3 V, inout float4 color)
{
	const float4 fog = GetFog(distance, GetCamera().position, -V);
	//color.rgb = (1.0 - fog.a) * color.rgb + fog.rgb; // premultiplied fog
	color.rgb = lerp(color.rgb, fog.rgb, fog.a); // non-premultiplied fog
}

inline void ApplyAerialPerspective(float2 uv, float3 P, inout float4 color)
{
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY_AERIAL_PERSPECTIVE)
	{
		const float4 AP = GetAerialPerspectiveTransmittance(uv, P, GetCamera().position, texture_cameravolumelut);
		color.rgb = color.rgb * (1.0 - AP.a) + AP.rgb;
	}
}

inline uint AlphaToCoverage(float alpha, float alphaTest, float4 svposition)
{
	if (alphaTest == 0)
	{
		// No alpha test, force full coverage:
		return ~0u;
	}

	if (GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED)
	{
		// When Temporal AA is enabled, dither the alpha mask with animated blue noise:
		alpha -= blue_noise(svposition.xy, svposition.w).x / GetCamera().sample_count;
	}
	else if (GetCamera().sample_count > 1)
	{
		// Without Temporal AA, use static dithering:
		alpha -= dither(svposition.xy) / GetCamera().sample_count;
	}
	else
	{
		// Without Temporal AA and MSAA, regular alpha test behaviour will be used:
		alpha -= alphaTest;
	}

	if (alpha > 0)
	{
		return ~0u >> (31u - uint(alpha * GetCamera().sample_count));
	}
	return 0;
}

#endif // WI_SHADING_HF
