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
		
	RWBuffer<float> wetmap = bindless_rwbuffers_float[push.wetmap];

	float prev = 0;

	if(push.iteration > 0)
	{
		prev = wetmap[DTid];
	}
	
	float current = lerp(prev, 0, GetDeltaTime() * 0.2);
	
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
		}
	}
	
	wetmap[DTid] = current;
}
