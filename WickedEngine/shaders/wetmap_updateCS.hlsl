#include "globals.hlsli"

PUSHCONSTANT(push, WetmapPush);

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	ShaderMeshInstance meshinstance = load_instance(push.instanceID);
	ShaderGeometry geometry = load_geometry(meshinstance.geometryOffset);
	if(geometry.vb_pos_wind < 0)
		return;

	Buffer<float4> vb_pos_wind = bindless_buffers_float4[geometry.vb_pos_wind];
	float3 world_pos = vb_pos_wind[DTid].xyz;
	world_pos = mul(meshinstance.transform.GetMatrix(), float4(world_pos, 1)).xyz;

	float drying = 0.02;
	[branch]
	if(geometry.vb_nor >= 0)
	{
		Buffer<float4> vb_nor = bindless_buffers_float4[geometry.vb_nor];
		float3 normal = vb_nor[DTid].xyz;
		normal = mul(normal, meshinstance.quaternion);
		normal = normalize(normal);
		drying *= lerp(4, 1, pow(saturate(normal.y), 8)); // modulate drying speed based on surface slope
	}
		
	RWBuffer<float> wetmap = bindless_rwbuffers_float[push.wetmap];

	float prev = wetmap[DTid];
	float current = prev;

	bool drying_enabled = true;
	
	const ShaderOcean ocean = GetWeather().ocean;
	float3 ocean_pos = float3(world_pos.x, ocean.water_height, world_pos.z);
	[branch]
	if (ocean.texture_displacementmap >= 0)
	{
		const float2 ocean_uv = ocean_pos.xz * ocean.patch_size_rcp;
		Texture2D texture_displacementmap = bindless_textures[ocean.texture_displacementmap];
		const float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, ocean_uv, 0).xzy;
		ocean_pos += displacement;
		float water_depth = ocean_pos.y - world_pos.y;
		if (water_depth > 0)
		{
			float shore = saturate(exp(-(ocean_pos.y - world_pos.y) / meshinstance.radius * 100)); // note: instance radius will soften the shoreline, looks better with large triangles like terrain
			current = max(current, smoothstep(0, 1.4, 1 - shore));
			drying_enabled=false;
		}
	}

	if(push.rain_amount > 0 && GetFrame().texture_shadowatlas_index >= 0 && any(GetFrame().rain_blocker_mad))
	{
		Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
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
