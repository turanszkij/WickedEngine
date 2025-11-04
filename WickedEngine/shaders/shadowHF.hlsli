#ifndef WI_SHADOW_HF
#define WI_SHADOW_HF
#include "globals.hlsli"

#define SHADOW_SAMPLING_DISK			// enables disk pattern sampling of shadows, otherwise it uses a fixed grid kernel
//#define SHADOW_SAMPLING_DITHERING		// enables dithering and temporal dithering for sampling, with this, shadows can use lower sample count but noise might be visible
//#define SHADOW_SAMPLING_PCSS			// enables penumbra computation based on blocker search (percentage closer soft shadows)

#ifdef SHADOW_SAMPLING_DISK

// "Vogel disk" sampling pattern based on: https://github.com/corporateshark/poisson-disk-generator/blob/master/PoissonGenerator.h
//	Baked values are remapped from [0, 1] range into [-1, 1] range by doing: value * 2 - 1
//	To change sampling count, simply uncomment the block that you want and comment the other blocks
static const half2 vogel_points[] = {

	//// 4 sample version:
	//half2(0.353553, 0.000000),
	//half2(-0.451560, 0.413635),
	//half2(0.069174, -0.787537),
	//half2(0.569060, 0.742409),

	//// 8 sample version:
	//float2(0.25000000, 0.00000000),
	//float2(-0.31930089, 0.29248416),
	//float2(0.04891348, -0.55687296),
	//float2(0.40238643, 0.52496207),
	//float2(-0.73851585, -0.13074535),
	//float2(0.69968677, -0.44490278),
	//float2(-0.23419666, 0.87043202),
	//float2(-0.44604915, -0.85938364),

	// 16 sample version:
	float2(0.17677665, 0.00000000),
	float2(-0.22577983, 0.20681751),
	float2(0.03458714, -0.39376867),
	float2(0.28453016, 0.37120426),
	float2(-0.52220953, -0.09245092),
	float2(0.49475324, -0.31459379),
	float2(-0.16560209, 0.61548841),
	float2(-0.31540442, -0.60767603),
	float2(0.68456841, 0.25023210),
	float2(-0.71235347, 0.29377294),
	float2(0.34362423, -0.73360229),
	float2(0.25340176, 0.80903494),
	float2(-0.76454973, -0.44352412),
	float2(0.89722824, -0.19680285),
	float2(-0.54790950, 0.77848911),
	float2(-0.12594837, -0.97615927),

	//// 32 sample version:
	//float2(0.12500000, 0.00000000),
	//float2(-0.15965044, 0.14624202),
	//float2(0.02445674, -0.27843648),
	//float2(0.20119321, 0.26248097),
	//float2(-0.36925793, -0.06537271),
	//float2(0.34984338, -0.22245139),
	//float2(-0.11709833, 0.43521595),
	//float2(-0.22302461, -0.42969179),
	//float2(0.48406291, 0.17694080),
	//float2(-0.50370997, 0.20772886),
	//float2(0.24297905, -0.51873517),
	//float2(0.17918217, 0.57207417),
	//float2(-0.54061830, -0.31361890),
	//float2(0.63443625, -0.13916063),
	//float2(-0.38743055, 0.55047488),
	//float2(-0.08905894, -0.69024885),
	//float2(0.54879880, 0.46308208),
	//float2(-0.73889750, 0.03009081),
	//float2(0.53931046, -0.53597510),
	//float2(-0.03660476, 0.77976608),
	//float2(-0.51236552, -0.61490381),
	//float2(0.81227481, 0.10993028),
	//float2(-0.68869931, 0.47834957),
	//float2(0.18879366, -0.83590192),
	//float2(0.43436146, 0.75957572),
	//float2(-0.85019755, -0.27210116),
	//float2(0.82646751, -0.38088906),
	//float2(-0.35873961, 0.85479879),
	//float2(-0.31848884, -0.88836360),
	//float2(0.84942913, 0.44759941),
	//float2(-0.94430852, 0.24780297),
	//float2(0.53754807, -0.83391672),

	//// 64 sample version:
	//float2(0.08838832, 0.00000000),
	//float2(-0.11288989, 0.10340881),
	//float2(0.01729357, -0.19688433),
	//float2(0.14226508, 0.18560219),
	//float2(-0.26110476, -0.04622549),
	//float2(0.24737668, -0.15729690),
	//float2(-0.08280104, 0.30774426),
	//float2(-0.15770221, -0.30383801),
	//float2(0.34228420, 0.12511611),
	//float2(-0.35617673, 0.14688647),
	//float2(0.17181218, -0.36680114),
	//float2(0.12670088, 0.40451741),
	//float2(-0.38227487, -0.22176206),
	//float2(0.44861412, -0.09840143),
	//float2(-0.27395475, 0.38924456),
	//float2(-0.06297421, -0.48807967),
	//float2(0.38805938, 0.32744849),
	//float2(-0.52247941, 0.02127743),
	//float2(0.38135004, -0.37899160),
	//float2(-0.02588350, 0.55137777),
	//float2(-0.36229712, -0.43480265),
	//float2(0.57436502, 0.07773244),
	//float2(-0.48698390, 0.33824420),
	//float2(0.13349736, -0.59107190),
	//float2(0.30713987, 0.53710103),
	//float2(-0.60118049, -0.19240457),
	//float2(0.58440077, -0.26932925),
	//float2(-0.25366724, 0.60443401),
	//float2(-0.22520560, -0.62816793),
	//float2(0.60063708, 0.31650054),
	//float2(-0.66772699, 0.17522311),
	//float2(0.38010383, -0.58966815),
	//float2(0.11987054, 0.70245540),
	//float2(-0.57146782, -0.44369137),
	//float2(0.73177743, -0.05970073),
	//float2(-0.50646347, 0.54606068),
	//float2(0.00468481, -0.75517583),
	//float2(0.51353145, 0.56764686),
	//float2(-0.77219379, -0.07265866),
	//float2(0.62647057, -0.47404855),
	//float2(-0.14353156, 0.78243923),
	//float2(-0.42785490, -0.68218595),
	//float2(0.78558588, 0.21660399),
	//float2(-0.73408145, 0.37524915),
	//float2(0.29113102, -0.78138036),
	//float2(0.31661296, 0.78146899),
	//float2(-0.76964480, -0.36634594),
	//float2(0.82370150, -0.25239521),
	//float2(-0.44148487, 0.75026906),
	//float2(-0.18308842, -0.86018378),
	//float2(0.72322643, 0.51575768),
	//float2(-0.89036399, 0.10926771),
	//float2(0.58837402, -0.68856990),
	//float2(0.03153503, 0.91375208),
	//float2(-0.64641660, -0.65856522),
	//float2(0.92991614, 0.04943299),
	//float2(-0.72555935, 0.59697247),
	//float2(0.13294542, -0.93848974),
	//float2(0.54051471, 0.78861046),
	//float2(-0.93917644, -0.21825480),
	//float2(0.84699202, -0.47740650),
	//float2(-0.30459112, 0.93175197),
	//float2(-0.40804350, -0.90003496),
	//float2(0.91606736, 0.39116228),
};

static const min16uint soft_shadow_sample_count = arraysize(vogel_points);
static const half soft_shadow_sample_count_rcp = rcp(soft_shadow_sample_count);

inline half3 sample_shadow(float2 uv, float cmp, float4 uv_clamping, half2 radius, min16uint2 pixel)
{
	Texture2D<float> texture_shadowatlas = bindless_textures_float[descriptor_index(GetFrame().texture_shadowatlas_index)];
	Texture2D<half4> texture_shadowatlas_transparent = bindless_textures_half4[descriptor_index(GetFrame().texture_shadowatlas_transparent_index)];
	
	half3 shadow = 0;
	
#ifndef DISABLE_SOFT_SHADOWMAP
	float2 spread = GetFrame().shadow_atlas_resolution_rcp.xy * mad(radius, 8, 2); // remap radius to try to match ray traced shadow result

#ifdef SHADOW_SAMPLING_PCSS
	half z_receiver = cmp;
	half blocker_count = 0;
	half blocker_sum = 0;
	const float search = 2;
	for (float x = -search; x <= search; ++x)
	for (float y = -search; y <= search; ++y)
	{
		const float2 sample_uv = clamp(mad(spread, float2(x, y) * 4, uv), uv_clamping.xy, uv_clamping.zw);
		const float4 depths = texture_shadowatlas.GatherRed(sampler_linear_clamp, sample_uv);
		for (uint d = 0; d < 4; ++d)
		{
			const half depth = (half)depths[d];
			if (depth > z_receiver)
			{
				blocker_count++;
				blocker_sum += depth;
			}
		}
	}
	if (blocker_count > 0)
	{
		half blocker_average = blocker_sum / blocker_count;
		half penumbra = abs(z_receiver - blocker_average);
		penumbra *= 200;
		spread = clamp(spread * penumbra, GetFrame().shadow_atlas_resolution_rcp.xy * 2, spread * 4);
	}
#endif // SHADOW_SAMPLING_PCSS
	
#ifdef SHADOW_SAMPLING_DITHERING
	const half2x2 rot = dither_rot2x2(pixel + GetTemporalAASampleRotation()); // per pixel rotation for every sample
#endif // SHADOW_SAMPLING_DITHERING
	[loop] // decent perf win with loop on the "island" level on AMD
	for (min16uint i = 0; i < soft_shadow_sample_count; ++i)
	{
#ifdef SHADOW_SAMPLING_DITHERING
		float2 sample_uv = mad(mul(vogel_points[i], rot), spread, uv);
#else
		float2 sample_uv = mad(vogel_points[i], spread, uv);
#endif // SHADOW_SAMPLING_DITHERING
#else
		float2 sample_uv = uv;
#endif // DISABLE_SOFT_SHADOWMAP

		sample_uv = clamp(sample_uv, uv_clamping.xy, uv_clamping.zw);
		half pcf_scalar = (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, sample_uv, cmp);
		half3 pcf = half3(pcf_scalar, pcf_scalar, pcf_scalar);
		
#ifndef DISABLE_TRANSPARENT_SHADOWMAP
		half4 transparent_shadow = texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, sample_uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		if (transparent_shadow.a > cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		{
			pcf *= transparent_shadow.rgb;
		}
#endif // DISABLE_TRANSPARENT_SHADOWMAP

		shadow += pcf;
		
#ifndef DISABLE_SOFT_SHADOWMAP
	}
	shadow *= soft_shadow_sample_count_rcp;
#endif // DISABLE_SOFT_SHADOWMAP

	return shadow;
}

// This is used to clamp the uvs to last texel center to avoid sampling on the border and overfiltering into a different shadow
inline float4 shadow_border_clamp(in ShaderEntity light, in float slice)
{
	const float2 border_size = 0.75 * GetFrame().shadow_atlas_resolution_rcp;
	const float2 topleft = mad(float2(slice, 0), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) + border_size;
	const float2 bottomright = mad(float2(slice + 1, 1), light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw) - border_size;
	return float4(topleft, bottomright);
}

inline half3 shadow_2D(in ShaderEntity light, in float z, in float2 shadow_uv, in uint cascade, min16uint2 pixel = 0)
{
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, z, shadow_border_clamp(light, cascade), light.GetType() == ENTITY_TYPE_RECTLIGHT ? (half2(light.GetRadius(), light.GetLength()) * 0.025) : light.GetRadius(), pixel);
}

inline half3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized, min16uint2 pixel = 0)
{
	const float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, remapped_distance, shadow_border_clamp(light, uv_slice.z), light.GetRadius(), pixel);
}

#else

inline half3 sample_shadow(float2 uv, float cmp, min16uint2 pixel)
{
	Texture2D<float> texture_shadowatlas = bindless_textures_float[descriptor_index(GetFrame().texture_shadowatlas_index)];
	half3 shadow = (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp);

#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a rectangle pattern around center:
	half sum = shadow.x;
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, -1));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, 0));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(-1, 1));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(0, -1));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(0, 1));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, -1));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, 0));
	sum += (half)texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp, int2(1, 1));
	shadow = half3(sum / 9.0, sum / 9.0, sum / 9.0);
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	Texture2D<half4> texture_shadowatlas_transparent = bindless_textures_half4[descriptor_index(GetFrame().texture_shadowatlas_transparent_index)];
	half4 transparent_shadow = texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	if (transparent_shadow.a > cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
	{
		shadow *= transparent_shadow.rgb;
	}
#endif //DISABLE_TRANSPARENT_SHADOWMAP

	return shadow;
}

// This is used to clamp the uvs to last texel center to avoid sampling on the border and overfiltering into a different shadow
inline void shadow_border_shrink(in ShaderEntity light, inout float2 shadow_uv)
{
	const float2 shadow_resolution = light.shadowAtlasMulAdd.xy * GetFrame().shadow_atlas_resolution;
#ifdef DISABLE_SOFT_SHADOWMAP
	const float border_size = 0.5;
#else
	const float border_size = 1.5;
#endif // DISABLE_SOFT_SHADOWMAP
	shadow_uv = clamp(shadow_uv * shadow_resolution, border_size, shadow_resolution - border_size) / shadow_resolution;
}

inline half3 shadow_2D(in ShaderEntity light, in float z, in float2 shadow_uv, in uint cascade, in min16uint2 pixel = 0)
{
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += cascade;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, z, pixel);
}

inline half3 shadow_cube(in ShaderEntity light, in float3 Lunnormalized, in min16uint2 pixel = 0)
{
	const float remapped_distance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 uv_slice = cubemap_to_uv(-Lunnormalized);
	float2 shadow_uv = uv_slice.xy;
	shadow_border_shrink(light, shadow_uv);
	shadow_uv.x += uv_slice.z;
	shadow_uv = mad(shadow_uv, light.shadowAtlasMulAdd.xy, light.shadowAtlasMulAdd.zw);
	return sample_shadow(shadow_uv, remapped_distance, pixel);
}

#endif // SHADOW_SAMPLING_DISK

inline half shadow_2D_volumetricclouds(float3 P)
{
	// Project into shadow map space (no need to divide by .w because ortho projection!):
	float3 shadow_pos = mul(GetFrame().cloudShadowLightSpaceMatrix, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	
	[branch]
	if (is_saturated(shadow_uv))
	{
		float cloudShadowSampleZ = shadow_pos.z;

		Texture2D<half4> texture_volumetricclouds_shadow = bindless_textures_half4[descriptor_index(GetFrame().texture_volumetricclouds_shadow_index)];
		half3 cloudShadowData = texture_volumetricclouds_shadow.SampleLevel(sampler_linear_clamp, shadow_uv.xy, 0.0).rgb;

		half sampleDepthKm = saturate(1.0 - cloudShadowSampleZ) * GetFrame().cloudShadowFarPlaneKm;
		
		half opticalDepth = cloudShadowData.g * (max(0.0, cloudShadowData.r - sampleDepthKm) * SKY_UNIT_TO_M);
		opticalDepth = min(cloudShadowData.b, opticalDepth);

		half transmittance = saturate(exp(-opticalDepth));
		return transmittance;
	}

	return 1.0;
}

// Sample light and furthest cascade for large mediums (volumetrics)
// Used with SkyAtmosphere and Volumetric Clouds
inline bool furthest_cascade_volumetrics(inout ShaderEntity light, inout uint furthestCascade)
{
	light = load_entity(lights().first_item() + GetWeather().most_important_light_index);
	furthestCascade = light.GetShadowCascadeCount() - 1;
	
	if (!light.IsStaticLight() && light.IsCastingShadow() && furthestCascade >= 0)
	{
		// We consider this light useless if it is static, is not casting shadow and if there are no available cascades
		return true;
	}
	
	return false;
}

static const float rain_blocker_head_size = 1;
static const float rain_blocker_head_size_sq = rain_blocker_head_size * rain_blocker_head_size;

// Checks whether position is below or above rain blocker
//	true: below
//	false: above
inline bool rain_blocker_check(in float3 P)
{
	// Check ocean blocker:
	const ShaderOcean ocean = GetWeather().ocean;
	if (ocean.texture_displacementmap >= 0)
	{
		Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		float2 ocean_uv = P.xz * ocean.patch_size_rcp;
		float3 displacement = displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		float water_height = ocean.water_height + displacement.y;
		if (P.y < water_height)
		{
			return true;
		}
	}

	// Before checking blocker shadow map, check "head" blocker:
	if(P.y < GetCamera().position.y + rain_blocker_head_size)
	{
		float2 diff = GetCamera().position.xz - P.xz;
		float distsq = dot(diff, diff);
		if(distsq < rain_blocker_head_size_sq)
			return true;
	}

	[branch]
	if (GetFrame().texture_shadowatlas_index < 0 || !any(GetFrame().rain_blocker_mad))
		return false;
		
	Texture2D<float> texture_shadowatlas = bindless_textures_float[descriptor_index(GetFrame().texture_shadowatlas_index)];
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad.xy, GetFrame().rain_blocker_mad.zw);
		float shadow = texture_shadowatlas.SampleLevel(sampler_point_clamp, shadow_uv.xy, 0);

		if(shadow > shadow_pos.z)
		{
			return true;
		}
		
	}
	return false;
}

// Same as above but using previous frame's values
inline bool rain_blocker_check_prev(in float3 P)
{
	// Check ocean blocker:
	const ShaderOcean ocean = GetWeather().ocean;
	if (ocean.texture_displacementmap >= 0)
	{
		Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
		float2 ocean_uv = P.xz * ocean.patch_size_rcp;
		float3 displacement = displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		float water_height = ocean.water_height + displacement.y;
		if (P.y < water_height)
		{
			return true;
		}
	}
	
	// Before checking blocker shadow map, check "head" blocker:
	if(P.y < GetCamera().position.y + rain_blocker_head_size)
	{
		float2 diff = GetCamera().position.xz - P.xz;
		float distsq = dot(diff, diff);
		if(distsq < rain_blocker_head_size_sq)
			return true;
	}
		
	[branch]
	if (GetFrame().texture_shadowatlas_index < 0 || !any(GetFrame().rain_blocker_mad_prev))
		return false;
		
	Texture2D<float> texture_shadowatlas = bindless_textures_float[descriptor_index(GetFrame().texture_shadowatlas_index)];
	float3 shadow_pos = mul(GetFrame().rain_blocker_matrix_prev, float4(P, 1)).xyz;
	float3 shadow_uv = clipspace_to_uv(shadow_pos);
	if (is_saturated(shadow_uv))
	{
		shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad_prev.xy, GetFrame().rain_blocker_mad_prev.zw);
		float shadow = texture_shadowatlas.SampleLevel(sampler_point_clamp, shadow_uv.xy, 0);

		if(shadow > shadow_pos.z)
		{
			return true;
		}
		
	}
	return false;
}

#endif // WI_SHADOW_HF
