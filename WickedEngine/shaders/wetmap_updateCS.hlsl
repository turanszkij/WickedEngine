#include "oceanHF.hlsli"

PUSHCONSTANT(push, WetmapPush);

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	ShaderMeshInstance meshinstance = load_instance(push.instanceID);
	ShaderGeometry geometry = load_geometry(meshinstance.geometryOffset);
	if(geometry.vb_pos_wind < 0)
		return;

	Buffer<float4> vb_pos_wind = bindless_buffers_float4[descriptor_index(geometry.vb_pos_wind)];
	float3 world_pos = vb_pos_wind[DTid].xyz;
	world_pos = mul(meshinstance.transform.GetMatrix(), float4(world_pos, 1)).xyz;

	float drying = 0.02;
	[branch]
	if(geometry.vb_nor >= 0)
	{
		Buffer<half4> vb_nor = bindless_buffers_half4[descriptor_index(geometry.vb_nor)];
		half3 normal = vb_nor[DTid].xyz;
		normal = mul(meshinstance.transformRaw.GetMatrixAdjoint(), normal);
		normal = normalize(normal);
		drying *= lerp(4, 1, pow8(saturate(normal.y))); // modulate drying speed based on surface slope
	}
		
	RWBuffer<float> wetmap = bindless_rwbuffers_float[descriptor_index(push.wetmap)];

	float prev = wetmap[DTid];
	float current = prev;

	bool drying_enabled = true;
	
	const ShaderOcean ocean = GetWeather().ocean;
	bool ocean_shelters_from_rain = false;
	[branch]
	if (ocean.IsValid())
	{
		const float height = ocean_height_at(ocean, world_pos.xz);
		const float water_depth = height - world_pos.y;
		if (water_depth > 0)
		{
			if (!ocean_exclusion_check(world_pos))
			{
				// Instance radius softens the shoreline for large triangles such as terrain.
				const float shore = saturate(exp(-water_depth / max(meshinstance.radius, 1e-4) * 100));
				current = max(current, smoothstep(0, 1.4, 1 - shore));
				drying_enabled = false;
			}
			else if (push.rain_amount > 0)
			{
				ocean_shelters_from_rain = ocean_blocks_rain(world_pos, height);
			}
		}
	}

	if(!ocean_shelters_from_rain && push.rain_amount > 0 && GetFrame().texture_shadowatlas_index >= 0 && any(GetFrame().rain_blocker_mad))
	{
		Texture2D texture_shadowatlas = bindless_textures[descriptor_index(GetFrame().texture_shadowatlas_index)];
		float3 shadow_pos = mul(GetFrame().rain_blocker_matrix, float4(world_pos, 1)).xyz;
		float3 shadow_uv = clipspace_to_uv(shadow_pos);
		float shadow = 1;
		if (is_saturated(shadow_uv))
		{
			shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad.xy, GetFrame().rain_blocker_mad.zw);

			float cmp = shadow_pos.z + 0.001;
			shadow  = texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(-1, -1)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(-1, 0)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(-1, 1)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(0, -1)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(0, 1)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(1, -1)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(1, 0)).r;
			shadow += texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, shadow_uv.xy, cmp, 2 * int2(1, 1)).r;
			shadow /= 9.0;
		}

		if(shadow == 0)
			drying_enabled = false;
		
		current = lerp(current, smoothstep(0, 1.4, sqrt(shadow)), saturate(GetDeltaTime() * 0.5));
	}

	if(drying_enabled)
		current = lerp(current, 0, GetDeltaTime() * drying);
	
	wetmap[DTid] = current;
}
