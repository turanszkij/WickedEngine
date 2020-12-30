#ifndef WI_OBJECTSHADER_HF
#define WI_OBJECTSHADER_HF

#ifdef TRANSPARENT
#define DISABLE_TRANSPARENT_SHADOWMAP
#endif

#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif

#ifdef WATER
#define DISABLE_VOXELGI
#endif

#define LIGHTMAP_QUALITY_BICUBIC


#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "brdf.hlsli"
#include "lightingHF.hlsli"

// DEFINITIONS
//////////////////

// These are bound by wiRenderer (based on Material):
TEXTURE2D(texture_basecolormap, float4, TEXSLOT_RENDERER_BASECOLORMAP);			// rgb: baseColor, a: opacity
TEXTURE2D(texture_normalmap, float3, TEXSLOT_RENDERER_NORMALMAP);				// rgb: normal
TEXTURE2D(texture_surfacemap, float4, TEXSLOT_RENDERER_SURFACEMAP);				// r: occlusion, g: roughness, b: metallic, a: reflectance
TEXTURE2D(texture_emissivemap, float4, TEXSLOT_RENDERER_EMISSIVEMAP);			// rgba: emissive
TEXTURE2D(texture_displacementmap, float, TEXSLOT_RENDERER_DISPLACEMENTMAP);	// r: heightmap
TEXTURE2D(texture_occlusionmap, float, TEXSLOT_RENDERER_OCCLUSIONMAP);			// r: occlusion
TEXTURE2D(texture_transmissionmap, float, TEXSLOT_RENDERER_TRANSMISSIONMAP);			// r: occlusion

TEXTURE2D(texture_blend1_basecolormap, float4, TEXSLOT_RENDERER_BLEND1_BASECOLORMAP);	// rgb: baseColor, a: opacity
TEXTURE2D(texture_blend1_normalmap, float3, TEXSLOT_RENDERER_BLEND1_NORMALMAP);			// rgb: normal
TEXTURE2D(texture_blend1_surfacemap, float4, TEXSLOT_RENDERER_BLEND1_SURFACEMAP);		// r: occlusion, g: roughness, b: metallic, a: reflectance
TEXTURE2D(texture_blend1_emissivemap, float4, TEXSLOT_RENDERER_BLEND1_EMISSIVEMAP);		// rgba: emissive

TEXTURE2D(texture_blend2_basecolormap, float4, TEXSLOT_RENDERER_BLEND2_BASECOLORMAP);	// rgb: baseColor, a: opacity
TEXTURE2D(texture_blend2_normalmap, float3, TEXSLOT_RENDERER_BLEND2_NORMALMAP);			// rgb: normal
TEXTURE2D(texture_blend2_surfacemap, float4, TEXSLOT_RENDERER_BLEND2_SURFACEMAP);		// r: occlusion, g: roughness, b: metallic, a: reflectance
TEXTURE2D(texture_blend2_emissivemap, float4, TEXSLOT_RENDERER_BLEND2_EMISSIVEMAP);		// rgba: emissive

TEXTURE2D(texture_blend3_basecolormap, float4, TEXSLOT_RENDERER_BLEND3_BASECOLORMAP);	// rgb: baseColor, a: opacity
TEXTURE2D(texture_blend3_normalmap, float3, TEXSLOT_RENDERER_BLEND3_NORMALMAP);			// rgb: normal
TEXTURE2D(texture_blend3_surfacemap, float4, TEXSLOT_RENDERER_BLEND3_SURFACEMAP);		// r: occlusion, g: roughness, b: metallic, a: reflectance
TEXTURE2D(texture_blend3_emissivemap, float4, TEXSLOT_RENDERER_BLEND3_EMISSIVEMAP);		// rgba: emissive

// These are bound by RenderPath (based on Render Path):
STRUCTUREDBUFFER(EntityTiles, uint, TEXSLOT_RENDERPATH_ENTITYTILES);
TEXTURE2D(texture_reflection, float4, TEXSLOT_RENDERPATH_REFLECTION);		// rgba: scene color from reflected camera angle
TEXTURE2D(texture_refraction, float4, TEXSLOT_RENDERPATH_REFRACTION);		// rgba: scene color from primary camera angle
TEXTURE2D(texture_waterriples, float4, TEXSLOT_RENDERPATH_WATERRIPPLES);	// rgb: snorm8 water ripple normal map
TEXTURE2D(texture_ao, float, TEXSLOT_RENDERPATH_AO);						// r: ambient occlusion
TEXTURE2D(texture_ssr, float4, TEXSLOT_RENDERPATH_SSR);						// rgb: screen space ray-traced reflections, a: reflection blend based on ray hit or miss


struct PixelInputType_Simple
{
	float4 pos		: SV_POSITION;
	float  clip		: SV_ClipDistance0;
	float4 color	: COLOR;
	float4 uvsets	: UVSETS;
};
struct PixelInputType
{
	float4 pos		 : SV_POSITION;
	float  clip		 : SV_ClipDistance0;
	float4 color	 : COLOR;
	float4 uvsets	 : UVSETS;
	float2 atl		 : ATLAS;
	float3 nor		 : NORMAL;
	float4 tan		 : TANGENT;
	float3 pos3D	 : WORLDPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

struct GBUFFEROutputType
{
	float4 g0	: SV_Target0;		// texture_gbuffer0
	float4 g1	: SV_Target1;		// texture_gbuffer1
};
inline GBUFFEROutputType CreateGbuffer(in float4 color, in Surface surface, in float2 velocity, in Lighting lighting)
{
	GBUFFEROutputType Out;
	Out.g0 = float4(color.rgb, surface.roughness);		/*FORMAT_R16G16B16A16_FLOAT*/
	Out.g1 = float4(encodeNormal(surface.N), velocity);	/*FORMAT_R16G16B16A16_FLOAT*/
	return Out;
}


// METHODS
////////////

inline void ApplyEmissive(in Surface surface, inout Lighting lighting)
{
	lighting.direct.specular += surface.emissiveColor.rgb * surface.emissiveColor.a;
}

inline void LightMapping(in float2 ATLAS, inout Lighting lighting)
{
	[branch]
	if (any(ATLAS))
	{
#ifdef LIGHTMAP_QUALITY_BICUBIC
		lighting.indirect.diffuse = SampleTextureCatmullRom(texture_globallightmap, sampler_linear_clamp, ATLAS).rgb;
#else
		lighting.indirect.diffuse = texture_globallightmap.SampleLevel(sampler_linear_clamp, ATLAS, 0).rgb;
#endif // LIGHTMAP_QUALITY_BICUBIC
	}
}

inline void NormalMapping(inout float4 uvsets, inout float3 N, in float3x3 TBN, out float3 bumpColor)
{
	[branch]
	if (g_xMaterial.normalMapStrength > 0 && g_xMaterial.uvset_normalMap >= 0)
	{
		const float2 UV_normalMap = g_xMaterial.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
		float3 normalMap = texture_normalmap.Sample(sampler_objectshader, UV_normalMap).rgb;
		bumpColor = normalMap.rgb * 2 - 1;
		N = normalize(lerp(N, mul(bumpColor, TBN), g_xMaterial.normalMapStrength));
		bumpColor *= g_xMaterial.normalMapStrength;
	}
	else
	{
		bumpColor = 0;
	}
}

inline float3 PlanarReflection(in Surface surface, in float2 bumpColor)
{
	float4 reflectionUV = mul(g_xCamera_ReflVP, float4(surface.P, 1));
	reflectionUV.xy /= reflectionUV.w;
	reflectionUV.xy = reflectionUV.xy * float2(0.5, -0.5) + 0.5;
	return texture_reflection.SampleLevel(sampler_linear_clamp, reflectionUV.xy + bumpColor*g_xMaterial.normalMapStrength, 0).rgb;
}

#define NUM_PARALLAX_OCCLUSION_STEPS 32
inline void ParallaxOcclusionMapping(inout float4 uvsets, in float3 V, in float3x3 TBN)
{
	[branch]
	if (g_xMaterial.parallaxOcclusionMapping > 0)
	{
		V = mul(TBN, V);
		float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
		float curLayerHeight = 0;
		float2 dtex = g_xMaterial.parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
		float2 originalTextureCoords = g_xMaterial.uvset_displacementMap == 0 ? uvsets.xy : uvsets.zw;
		float2 currentTextureCoords = originalTextureCoords;
		float2 derivX = ddx_coarse(currentTextureCoords);
		float2 derivY = ddy_coarse(currentTextureCoords);
		float heightFromTexture = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
		uint iter = 0;
		[loop]
		while (heightFromTexture > curLayerHeight && iter < NUM_PARALLAX_OCCLUSION_STEPS)
		{
			curLayerHeight += layerHeight;
			currentTextureCoords -= dtex;
			heightFromTexture = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
			iter++;
		}
		float2 prevTCoords = currentTextureCoords + dtex;
		float nextH = heightFromTexture - curLayerHeight;
		float prevH = 1 - texture_displacementmap.SampleGrad(sampler_linear_wrap, prevTCoords, derivX, derivY).r - curLayerHeight + layerHeight;
		float weight = nextH / (nextH - prevH);
		float2 finalTextureCoords = prevTCoords * weight + currentTextureCoords * (1.0 - weight);
		float2 difference = finalTextureCoords - originalTextureCoords;
		uvsets += difference.xyxy;
	}
}

inline void ForwardLighting(inout Surface surface, inout Lighting lighting)
{
#ifndef DISABLE_DECALS
	[branch]
	if (xForwardDecalMask != 0)
	{
		// decals are enabled, loop through them first:
		float4 decalAccumulation = 0;
		const float3 P_dx = ddx_coarse(surface.P);
		const float3 P_dy = ddy_coarse(surface.P);

		uint bucket_bits = xForwardDecalMask;

		[loop]
		while (bucket_bits != 0)
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket_bit_index;
			bucket_bits ^= 1 << bucket_bit_index;

			[branch]
			if (decalAccumulation.a < 1)
			{
				ShaderEntity decal = EntityArray[g_xFrame_DecalArrayOffset + entity_index];
				if ((decal.layerMask & g_xMaterial.layerMask) == 0)
					continue;

				float4x4 decalProjection = MatrixArray[decal.GetMatrixIndex()];
				float4 texMulAdd = decalProjection[3];
				decalProjection[3] = float4(0, 0, 0, 1);
				const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
				const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
				[branch]
				if (is_saturated(uvw))
				{
					// mipmapping needs to be performed by hand:
					const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy * texMulAdd.xy;
					const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy * texMulAdd.xy;
					float4 decalColor = texture_decalatlas.SampleGrad(sampler_linear_clamp, uvw.xy * texMulAdd.xy + texMulAdd.zw, decalDX, decalDY);
					// blend out if close to cube Z:
					float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
					decalColor.a *= edgeBlend;
					decalColor *= decal.GetColor();
					// apply emissive:
					lighting.direct.specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
					// perform manual blending of decals:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
					decalAccumulation.rgb = (1 - decalAccumulation.a) * (decalColor.a*decalColor.rgb) + decalAccumulation.rgb;
					decalAccumulation.a = decalColor.a + (1 - decalColor.a) * decalAccumulation.a;
					[branch]
					if (decalAccumulation.a >= 1.0)
					{
						// force exit:
						break;
					}
				}
			}
			else
			{
				// force exit:
				break;
			}

		}

		surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
	}
#endif // DISABLE_DECALS


#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	float4 envmapAccumulation = 0;
	const float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;

#ifndef ENVMAPRENDERING
#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (xForwardEnvProbeMask != 0)
	{
		uint bucket_bits = xForwardEnvProbeMask;

		[loop]
		while (bucket_bits != 0)
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket_bit_index;
			bucket_bits ^= 1 << bucket_bit_index;

			[branch]
			if (envmapAccumulation.a < 1)
			{
				ShaderEntity probe = EntityArray[g_xFrame_EnvProbeArrayOffset + entity_index];
				if ((probe.layerMask & g_xMaterial.layerMask) == 0)
					continue;

				const float4x4 probeProjection = MatrixArray[probe.GetMatrixIndex()];
				const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
				const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
				[branch]
				if (is_saturated(uvw))
				{
					const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos, envMapMIP);
					// perform manual blending of probes:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
					envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
					envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
					[branch]
					if (envmapAccumulation.a >= 1.0)
					{
						// force exit:
						break;
					}
				}
			}
			else
			{
				// force exit:
				break;
			}

		}
	}
#endif // DISABLE_LOCALENVPMAPS
#endif //ENVMAPRENDERING

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface, envMapMIP), envmapAccumulation.rgb, envmapAccumulation.a);
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
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
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
				bucket_bits ^= 1 << bucket_bit_index;

				ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + entity_index];

				if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
				{
					continue; // static lights will be skipped (they are used in lightmap baking)
				}

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					DirectionalLight(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					PointLight(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					SpotLight(light, surface, lighting);
				}
				break;
				}
			}
		}
	}

}

inline void TiledLighting(inout Surface surface, inout Lighting lighting)
{
	const uint2 tileIndex = uint2(floor(surface.pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;


#ifndef DISABLE_DECALS
	[branch]
	if (g_xFrame_DecalArrayCount > 0)
	{
		// decals are enabled, loop through them first:
		float4 decalAccumulation = 0;
		const float3 P_dx = ddx_coarse(surface.P);
		const float3 P_dy = ddy_coarse(surface.P);

		// Loop through decal buckets in the tile:
		const uint first_item = g_xFrame_DecalArrayOffset;
		const uint last_item = first_item + g_xFrame_DecalArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// This is the wave scalarizer from Improved Culling - Siggraph 2017 [Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1 << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item && decalAccumulation.a < 1)
				{
					ShaderEntity decal = EntityArray[entity_index];
					if ((decal.layerMask & g_xMaterial.layerMask) == 0)
						continue;

					float4x4 decalProjection = MatrixArray[decal.GetMatrixIndex()];
					float4 texMulAdd = decalProjection[3];
					decalProjection[3] = float4(0, 0, 0, 1);
					const float3 clipSpacePos = mul(decalProjection, float4(surface.P, 1)).xyz;
					const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
					[branch]
					if (is_saturated(uvw))
					{
						// mipmapping needs to be performed by hand:
						const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy * texMulAdd.xy;
						const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy * texMulAdd.xy;
						float4 decalColor = texture_decalatlas.SampleGrad(sampler_linear_clamp, uvw.xy * texMulAdd.xy + texMulAdd.zw, decalDX, decalDY);
						// blend out if close to cube Z:
						float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
						decalColor.a *= edgeBlend;
						decalColor *= decal.GetColor();
						// apply emissive:
						lighting.direct.specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
						// perform manual blending of decals:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
						decalAccumulation.rgb = (1 - decalAccumulation.a) * (decalColor.a*decalColor.rgb) + decalAccumulation.rgb;
						decalAccumulation.a = decalColor.a + (1 - decalColor.a) * decalAccumulation.a;
						[branch]
						if (decalAccumulation.a >= 1.0)
						{
							// force exit:
							bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
							break;
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

		surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
	}
#endif // DISABLE_DECALS


#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	float4 envmapAccumulation = 0;
	const float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;

#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (g_xFrame_EnvProbeArrayCount > 0)
	{
		// Loop through envmap buckets in the tile:
		const uint first_item = g_xFrame_EnvProbeArrayOffset;
		const uint last_item = first_item + g_xFrame_EnvProbeArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1 << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item && envmapAccumulation.a < 1)
				{
					ShaderEntity probe = EntityArray[entity_index];
					if ((probe.layerMask & g_xMaterial.layerMask) == 0)
						continue;

					const float4x4 probeProjection = MatrixArray[probe.GetMatrixIndex()];
					const float3 clipSpacePos = mul(probeProjection, float4(surface.P, 1)).xyz;
					const float3 uvw = clipSpacePos.xyz * float3(0.5, -0.5, 0.5) + 0.5;
					[branch]
					if (is_saturated(uvw))
					{
						const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos, envMapMIP);
						// perform manual blending of probes:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
						envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
						envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
						[branch]
						if (envmapAccumulation.a >= 1.0)
						{
							// force exit:
							bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
							break;
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
	}
#endif // DISABLE_LOCALENVPMAPS
	
	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface, envMapMIP), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	lighting.indirect.specular += max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

#ifndef DISABLE_VOXELGI
	VoxelGI(surface, lighting);
#endif //DISABLE_VOXELGI

	[branch]
	if (g_xFrame_LightArrayCount > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = g_xFrame_LightArrayOffset;
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
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

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						DirectionalLight(light, surface, lighting);
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						PointLight(light, surface, lighting);
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						SpotLight(light, surface, lighting);
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

}

inline void ApplyLighting(in Surface surface, in Lighting lighting, inout float4 color)
{
	LightingPart combined_lighting = CombineLighting(surface, lighting);
	color.rgb = lerp(surface.albedo * combined_lighting.diffuse, surface.refraction.rgb, surface.refraction.a) + combined_lighting.specular;
}

inline void ApplyFog(in float dist, inout float4 color)
{
	float3 V = g_xFrame_Options & OPTION_BIT_REALISTIC_SKY ? float3(0.0, 1.0, 0.0) : float3(0.0, -1.0, 0.0);	
	color.rgb = lerp(color.rgb, GetDynamicSkyColor(V, false, false, false, true), GetFogAmount(dist));
}


// OBJECT SHADER PROTOTYPE
///////////////////////////

#if defined(COMPILE_OBJECTSHADER_PS)

// Possible switches:
//	ALPHATESTONLY		-	assemble object shader for depth only rendering + alpha test
//	TEXTUREONLY			-	assemble object shader for rendering only with base textures, no lighting
//	TRANSPARENT			-	assemble object shader for forward or tile forward transparent rendering
//	ENVMAPRENDERING		-	modify object shader for envmap rendering
//	PLANARREFLECTION	-	include planar reflection sampling
//	POM					-	include parallax occlusion mapping computation
//	WATER				-	include specialized water shader code
//	BLACKOUT			-	include specialized blackout shader code
//	TERRAIN				-	include specialized terrain material blending code

#if defined(ALPHATESTONLY) || defined(TEXTUREONLY)
#define SIMPLE_INPUT
#endif // APLHATESTONLY

#ifdef SIMPLE_INPUT
#define PIXELINPUT PixelInputType_Simple
#else
#define PIXELINPUT PixelInputType
#endif // SIMPLE_INPUT


#ifdef DISABLE_ALPHATEST
[earlydepthstencil]
#endif // DISABLE_ALPHATEST


// entry point:
#if defined(ALPHATESTONLY)
void main(PIXELINPUT input)
#elif defined(TEXTUREONLY)
float4 main(PIXELINPUT input) : SV_TARGET
#elif defined(TRANSPARENT)
float4 main(PIXELINPUT input) : SV_TARGET
#elif defined(ENVMAPRENDERING)
float4 main(PSIn_EnvmapRendering input) : SV_TARGET
#else
GBUFFEROutputType main(PIXELINPUT input)
#endif // ALPHATESTONLY



// shader base:
{
	float2 pixel = input.pos.xy;
	float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;

#ifndef DISABLE_ALPHATEST
#ifndef TRANSPARENT
#ifndef ENVMAPRENDERING
	// apply dithering:
	clip(dither(pixel + GetTemporalAASampleRotation()) - (1 - input.color.a));
#endif // DISABLE_ALPHATEST
#endif // TRANSPARENT
#endif // ENVMAPRENDERING

	Surface surface;

#ifndef SIMPLE_INPUT
	surface.P = input.pos3D;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length(surface.V);
	surface.V /= dist;
	surface.N = normalize(input.nor);

#if 0
	float3x3 TBN = compute_tangent_frame(surface.N, surface.P, input.uvsets.xy);
#else
	float4 tangent = input.tan;
	tangent.xyz = normalize(tangent.xyz);
	float3 binormal = normalize(cross(tangent.xyz, surface.N) * tangent.w);
	float3x3 TBN = float3x3(tangent.xyz, binormal, surface.N);
#endif

#endif // SIMPLE_INPUT

#ifdef POM
	ParallaxOcclusionMapping(input.uvsets, surface.V, TBN);
#endif // POM

	float4 color = 1;
	[branch]
	if (g_xMaterial.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
	}
	color *= input.color;

#ifndef DISABLE_ALPHATEST
	clip(color.a - g_xMaterial.alphaTest);
#endif // DISABLE_ALPHATEST

#ifndef SIMPLE_INPUT
	float3 bumpColor = 0;
	float depth = input.pos.z;
#ifndef ENVMAPRENDERING
	float2 pos2D = ScreenCoord * 2 - 1;
	pos2D.y *= -1;
	input.pos2DPrev.xy /= input.pos2DPrev.w;

	const float2 velocity = ((input.pos2DPrev.xy - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5, -0.5);
	const float2 ReprojectedScreenCoord = ScreenCoord + velocity;

#ifndef WATER
	NormalMapping(input.uvsets, surface.N, TBN, bumpColor);
#endif // WATER

#endif // ENVMAPRENDERING
#endif // SIMPLE_INPUT

	// Surface map:
	float4 surfaceMap = 1;
	[branch]
	if (g_xMaterial.uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = g_xMaterial.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surfaceMap = texture_surfacemap.Sample(sampler_objectshader, UV_surfaceMap);
	}

	surface.create(g_xMaterial, color, surfaceMap);

	// Emissive map:
	surface.emissiveColor = g_xMaterial.emissiveColor;
	[branch]
	if (surface.emissiveColor.a > 0 && g_xMaterial.uvset_emissiveMap >= 0)
	{
		const float2 UV_emissiveMap = g_xMaterial.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		float4 emissiveMap = texture_emissivemap.Sample(sampler_objectshader, UV_emissiveMap);
		emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
		surface.emissiveColor *= emissiveMap;
	}

#ifdef TERRAIN
	surface.N = 0;
	surface.albedo = 0;
	surface.f0 = 0;
	surface.roughness = 0;
	surface.occlusion = 0;
	surface.emissiveColor = 0;

	float4 sam;
	float4 blend_weights = input.color;
	blend_weights /= blend_weights.x + blend_weights.y + blend_weights.z + blend_weights.w;
	float3 baseN = normalize(input.nor);

	[branch]
	if (blend_weights.x > 0)
	{
		float4 color2 = 1;
		[branch]
		if (g_xMaterial.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			float2 uv = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			color2 = texture_basecolormap.Sample(sampler_objectshader, uv);
			color2.rgb = DEGAMMA(color2.rgb);
		}

		float4 surfaceMap2 = 1;
		[branch]
		if (g_xMaterial.uvset_surfaceMap >= 0)
		{
			float2 uv = g_xMaterial.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			surfaceMap2 = texture_surfacemap.Sample(sampler_objectshader, uv);
		}

		Surface surface2;
		surface2.N = baseN;
		surface2.create(g_xMaterial, color2, surfaceMap2);

		[branch]
		if (g_xMaterial.normalMapStrength > 0 && g_xMaterial.uvset_normalMap >= 0)
		{
			float2 uv = g_xMaterial.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam.rgb = texture_normalmap.Sample(sampler_objectshader, uv);
			sam.rgb = sam.rgb * 2 - 1;
			surface2.N = lerp(baseN, mul(sam.rgb, TBN), g_xMaterial.normalMapStrength);
		}

		surface2.emissiveColor = g_xMaterial.emissiveColor;
		[branch]
		if (g_xMaterial.uvset_emissiveMap >= 0 && any(g_xMaterial.emissiveColor))
		{
			float2 uv = g_xMaterial.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam = texture_emissivemap.Sample(sampler_objectshader, uv);
			sam.rgb = DEGAMMA(sam.rgb);
			surface2.emissiveColor *= sam;
		}

		surface.N += surface2.N * blend_weights.x;
		surface.albedo += surface2.albedo * blend_weights.x;
		surface.f0 += surface2.f0 * blend_weights.x;
		surface.roughness += surface2.roughness * blend_weights.x;
		surface.occlusion += surface2.occlusion * blend_weights.x;
		surface.emissiveColor += surface2.emissiveColor * blend_weights.x;
	}

	[branch]
	if (blend_weights.y > 0)
	{
		float4 color2 = 1;
		[branch]
		if (g_xMaterial_blend1.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			float2 uv = g_xMaterial_blend1.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			color2 = texture_blend1_basecolormap.Sample(sampler_objectshader, uv);
			color2.rgb = DEGAMMA(color2.rgb);
		}

		float4 surfaceMap2 = 1;
		[branch]
		if (g_xMaterial_blend1.uvset_surfaceMap >= 0)
		{
			float2 uv = g_xMaterial_blend1.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			surfaceMap2 = texture_blend1_surfacemap.Sample(sampler_objectshader, uv);
		}

		Surface surface2;
		surface2.N = baseN;
		surface2.create(g_xMaterial_blend1, color2, surfaceMap2);

		[branch]
		if (g_xMaterial_blend1.normalMapStrength > 0 && g_xMaterial_blend1.uvset_normalMap >= 0)
		{
			float2 uv = g_xMaterial_blend1.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam.rgb = texture_blend1_normalmap.Sample(sampler_objectshader, uv);
			sam.rgb = sam.rgb * 2 - 1;
			surface2.N = lerp(baseN, mul(sam.rgb, TBN), g_xMaterial_blend1.normalMapStrength);
		}

		surface2.emissiveColor = g_xMaterial_blend1.emissiveColor;
		[branch]
		if (g_xMaterial_blend1.uvset_emissiveMap >= 0 && any(g_xMaterial.emissiveColor))
		{
			float2 uv = g_xMaterial_blend1.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam = texture_blend1_emissivemap.Sample(sampler_objectshader, uv);
			sam.rgb = DEGAMMA(sam.rgb);
			surface2.emissiveColor *= sam;
		}

		surface.N += surface2.N * blend_weights.y;
		surface.albedo += surface2.albedo * blend_weights.y;
		surface.f0 += surface2.f0 * blend_weights.y;
		surface.roughness += surface2.roughness * blend_weights.y;
		surface.occlusion += surface2.occlusion * blend_weights.y;
		surface.emissiveColor += surface2.emissiveColor * blend_weights.y;
	}

	[branch]
	if (blend_weights.z > 0)
	{
		float4 color2 = 1;
		[branch]
		if (g_xMaterial_blend2.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			float2 uv = g_xMaterial_blend2.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			color2 = texture_blend2_basecolormap.Sample(sampler_objectshader, uv);
			color2.rgb = DEGAMMA(color2.rgb);
		}

		float4 surfaceMap2 = 1;
		[branch]
		if (g_xMaterial_blend2.uvset_surfaceMap >= 0)
		{
			float2 uv = g_xMaterial_blend2.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			surfaceMap2 = texture_blend2_surfacemap.Sample(sampler_objectshader, uv);
		}

		Surface surface2;
		surface2.N = baseN;
		surface2.create(g_xMaterial_blend2, color2, surfaceMap2);

		[branch]
		if (g_xMaterial_blend2.normalMapStrength > 0 && g_xMaterial_blend2.uvset_normalMap >= 0)
		{
			float2 uv = g_xMaterial_blend2.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam.rgb = texture_blend2_normalmap.Sample(sampler_objectshader, uv);
			sam.rgb = sam.rgb * 2 - 1;
			surface2.N = lerp(baseN, mul(sam.rgb, TBN), g_xMaterial_blend2.normalMapStrength);
		}

		surface2.emissiveColor = g_xMaterial_blend2.emissiveColor;
		[branch]
		if (g_xMaterial_blend2.uvset_emissiveMap >= 0 && any(g_xMaterial_blend2.emissiveColor))
		{
			float2 uv = g_xMaterial_blend2.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam = texture_blend2_emissivemap.Sample(sampler_objectshader, uv);
			sam.rgb = DEGAMMA(sam.rgb);
			surface2.emissiveColor *= sam;
		}

		surface.N += surface2.N * blend_weights.z;
		surface.albedo += surface2.albedo * blend_weights.z;
		surface.f0 += surface2.f0 * blend_weights.z;
		surface.roughness += surface2.roughness * blend_weights.z;
		surface.occlusion += surface2.occlusion * blend_weights.z;
		surface.emissiveColor += surface2.emissiveColor * blend_weights.z;
	}

	[branch]
	if (blend_weights.w > 0)
	{
		float4 color2 = 1;
		[branch]
		if (g_xMaterial_blend3.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			float2 uv = g_xMaterial_blend3.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			color2 = texture_blend3_basecolormap.Sample(sampler_objectshader, uv);
			color2.rgb = DEGAMMA(color2.rgb);
		}

		float4 surfaceMap2 = 1;
		[branch]
		if (g_xMaterial_blend3.uvset_surfaceMap >= 0)
		{
			float2 uv = g_xMaterial_blend3.uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			surfaceMap2 = texture_blend3_surfacemap.Sample(sampler_objectshader, uv);
		}

		Surface surface2;
		surface2.N = baseN;
		surface2.create(g_xMaterial_blend3, color2, surfaceMap2);

		[branch]
		if (g_xMaterial_blend3.normalMapStrength > 0 && g_xMaterial_blend3.uvset_normalMap >= 0)
		{
			float2 uv = g_xMaterial_blend3.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam.rgb = texture_blend3_normalmap.Sample(sampler_objectshader, uv);
			sam.rgb = sam.rgb * 2 - 1;
			surface2.N = lerp(baseN, mul(sam.rgb, TBN), g_xMaterial_blend3.normalMapStrength);
		}

		surface2.emissiveColor = g_xMaterial_blend3.emissiveColor;
		[branch]
		if (g_xMaterial_blend3.uvset_emissiveMap >= 0 && any(g_xMaterial_blend3.emissiveColor))
		{
			float2 uv = g_xMaterial_blend3.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			sam = texture_blend3_emissivemap.Sample(sampler_objectshader, uv);
			sam.rgb = DEGAMMA(sam.rgb);
			surface2.emissiveColor *= sam;
		}

		surface.N += surface2.N * blend_weights.w;
		surface.albedo += surface2.albedo * blend_weights.w;
		surface.f0 += surface2.f0 * blend_weights.w;
		surface.roughness += surface2.roughness * blend_weights.w;
		surface.occlusion += surface2.occlusion * blend_weights.w;
		surface.emissiveColor += surface2.emissiveColor * blend_weights.w;
	}

	surface.N = normalize(surface.N);

#endif // TERRAIN

	// Secondary occlusion map:
	[branch]
	if (g_xMaterial.IsOcclusionEnabled_Secondary() && g_xMaterial.uvset_occlusionMap >= 0)
	{
		const float2 UV_occlusionMap = g_xMaterial.uvset_occlusionMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surface.occlusion *= texture_occlusionmap.Sample(sampler_objectshader, UV_occlusionMap).r;
	}

#ifndef SIMPLE_INPUT
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	surface.occlusion *= texture_ao.SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // SIMPLE_INPUT

#ifdef BRDF_ANISOTROPIC
	surface.anisotropy = g_xMaterial.parallaxOcclusionMapping;
	surface.T = tangent.xyz;
	surface.B = normalize(cross(tangent.xyz, surface.N) * tangent.w); // Compute bitangent again after normal mapping
#endif // BRDF_ANISOTROPIC

	surface.sss = g_xMaterial.subsurfaceScattering;
	surface.sss_inv = g_xMaterial.subsurfaceScattering_inv;

	surface.pixel = pixel;
	surface.screenUV = ScreenCoord;

	surface.layerMask = g_xMaterial.layerMask;

	surface.update();

	float3 ambient = GetAmbient(surface.N);
	ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

	Lighting lighting;
	lighting.create(0, 0, ambient, 0);

#ifndef SIMPLE_INPUT


#ifdef WATER
	color.a = 1;

	//NORMALMAP
	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	[branch]
	if (g_xMaterial.uvset_normalMap >= 0)
	{
		const float2 UV_normalMap = g_xMaterial.uvset_normalMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		bumpColor0 = 2 * texture_normalmap.Sample(sampler_objectshader, UV_normalMap - g_xMaterial.texMulAdd.ww).rg - 1;
		bumpColor1 = 2 * texture_normalmap.Sample(sampler_objectshader, UV_normalMap + g_xMaterial.texMulAdd.zw).rg - 1;
	}
	bumpColor2 = texture_waterriples.SampleLevel(sampler_objectshader, ScreenCoord, 0).rg;
	bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * g_xMaterial.refraction;
	surface.N = normalize(lerp(surface.N, mul(normalize(bumpColor), TBN), g_xMaterial.normalMapStrength));
	bumpColor *= g_xMaterial.normalMapStrength;

	//REFLECTION
	float4 reflectionUV = mul(g_xCamera_ReflVP, float4(surface.P, 1));
	reflectionUV.xy /= reflectionUV.w;
	reflectionUV.xy = reflectionUV.xy * float2(0.5, -0.5) + 0.5;
	lighting.indirect.specular += texture_reflection.SampleLevel(sampler_linear_mirror, reflectionUV.xy + bumpColor.rg, 0).rgb;
#endif // WATER



#ifdef TRANSPARENT
	[branch]
	if (g_xMaterial.transmission > 0)
	{
		float transmission = g_xMaterial.transmission;
		[branch]
		if (g_xMaterial.uvset_transmissionMap >= 0)
		{
			const float2 UV_transmissionMap = g_xMaterial.uvset_transmissionMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			float transmissionMap = texture_transmissionmap.Sample(sampler_objectshader, UV_transmissionMap);
			transmission *= transmissionMap;
		}
		float2 size;
		float mipLevels;
		texture_refraction.GetDimensions(0, size.x, size.y, mipLevels);
		const float2 normal2D = mul((float3x3)g_xCamera_View, surface.N.xyz).xy;
		float2 perturbatedRefrTexCoords = ScreenCoord.xy + normal2D * g_xMaterial.refraction;
		float4 refractiveColor = texture_refraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, surface.roughness * mipLevels);
		surface.refraction.rgb = surface.albedo * refractiveColor.rgb;
		surface.refraction.a = transmission;
	}
#endif // TRANSPARENT



	LightMapping(input.atl, lighting);

	ApplyEmissive(surface, lighting);

#ifdef PLANARREFLECTION
	lighting.indirect.specular += PlanarReflection(surface, bumpColor.rg);
#endif




#ifdef FORWARD
	ForwardLighting(surface, lighting);
#endif // FORWARD

#ifdef TILEDFORWARD
	TiledLighting(surface, lighting);
#endif // TILEDFORWARD


#ifndef WATER
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	float4 ssr = texture_ssr.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb, ssr.a);
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // WATER

#ifdef WATER
	// WATER REFRACTION
	float lineardepth = input.pos.w;
	float sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy + bumpColor.rg, 0) * g_xCamera_ZFarP;
	float depth_difference = sampled_lineardepth - lineardepth;
	surface.refraction.rgb = texture_refraction.SampleLevel(sampler_linear_mirror, ScreenCoord.xy + bumpColor.rg * saturate(0.5 * depth_difference), 0).rgb;
	if (depth_difference < 0)
	{
		// Fix cutoff by taking unperturbed depth diff to fill the holes with fog:
		sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord.xy, 0) * g_xCamera_ZFarP;
		depth_difference = sampled_lineardepth - lineardepth;
	}
	// WATER FOG:
	surface.refraction.a = 1 - saturate(color.a * 0.1 * depth_difference);
#endif // WATER

#ifdef UNLIT
	lighting.direct.diffuse = 1;
	lighting.indirect.diffuse = 0;
	lighting.direct.specular = 0;
	lighting.indirect.specular = 0;
#endif // UNLIT

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, color);


#endif // SIMPLE_INPUT


#ifdef TEXTUREONLY
	color.rgb += surface.emissiveColor.rgb * surface.emissiveColor.a;
#endif // TEXTUREONLY


#ifdef BLACKOUT
	color = float4(0, 0, 0, 1);
#endif

	color = max(0, color);


	// end point:
#ifndef ALPHATESTONLY
#ifdef OUTPUT_GBUFFER
	return CreateGbuffer(color, surface, velocity, lighting);
#else
	return color;
#endif // OUTPUT_GBUFFER
#endif // ALPHATESTONLY

}


#endif // COMPILE_OBJECTSHADER_PS



#endif // WI_OBJECTSHADER_HF

