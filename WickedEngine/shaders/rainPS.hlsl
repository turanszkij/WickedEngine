#define TRANSPARENT // uses transparent light lists
#include "globals.hlsli"
#include "shadingHF.hlsli"

float4 main(
	in float4 pos : SV_Position,
	in float3 P : TEXCOORD1,
	in float3 uvw : TEXCOORD2
) : SV_Target
{
	uint2 pixel = pos.xy;

	ShaderWeather weather = GetWeather();
	
	float amount = 1 - weather.rain_amount;
				
	uvw /= weather.rain_scale;
	uvw.y /= weather.rain_length;
	uvw.y += GetTime() * weather.rain_speed;
	float noise = noise_gradient_3D(uvw);
	noise = noise - amount;
	noise = saturate(noise);
	//noise = smoothstep(0.3, 1, noise);
	float4 color = 1;
	color.a *= noise;

	float3 V = P - GetCamera().position;
	float dist = length(V);
	V /= dist;
	float top_fade = abs(V.y);
	top_fade = smoothstep(0.8,1,top_fade);
	color.a = lerp(color.a, 0, top_fade);
	//color = float4(1,1,1,top_fade);

	// Blocker shadow map check:
	[branch]
	if (GetFrame().texture_shadowatlas_index >= 0)
	{
		Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
		float3 shadow_pos = mul(GetFrame().rain_blocker_matrix, float4(P, 1)).xyz;
		float3 shadow_uv = clipspace_to_uv(shadow_pos);
		if (is_saturated(shadow_uv))
		{
			shadow_uv.xy = mad(shadow_uv.xy, GetFrame().rain_blocker_mad.xy, GetFrame().rain_blocker_mad.zw);
			float shadow = texture_shadowatlas.SampleLevel(sampler_linear_clamp, shadow_uv.xy, 0).r;

			if(shadow > shadow_pos.z)
			{
				color.a = 0;
			}

#if 1
			// Vapor effect:
			{
				float3 P2 = reconstruct_position(clipspace_to_uv(shadow_pos).xy, shadow, GetFrame().rain_blocker_matrix_inverse);
				float vapor_falloff = distance(P, P2);
				vapor_falloff *= 0.2;
				vapor_falloff = saturate(vapor_falloff);
				vapor_falloff = 1 - pow(1 - vapor_falloff, 4);
				float vapor_color = 1;
				float3 uvw = P - GetCamera().position;
				// small scale vapor noise:
				vapor_color += noise_gradient_3D(uvw + GetTime() * 0.1 * weather.rain_speed);
				vapor_color -= 0.5;
				// large scale vapor noise:
				vapor_color *= noise_gradient_3D(uvw * 0.04 + GetTime() * 0.5);
				vapor_color = saturate(vapor_color);
				color = lerp(vapor_color, color, vapor_falloff);
			}
#endif
		}
	}
	
	[branch]
	if (GetCamera().texture_lineardepth_index >= 0)
	{
		float2 ScreenCoord = pixel * GetCamera().internal_resolution_rcp;
		float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, ScreenCoord) * GetCamera().z_far;
		float depthFragment = pos.w;
		
		float softness_intersection = 4;
		float intersection_fade = saturate(rcp(softness_intersection) * (max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));
		color.a *= intersection_fade;
	}
	
	[branch]
	if (color.a > 0)
	{
		//float3 N = normalize(uvw);
		float3 N = V;

		Lighting lighting;
		lighting.create(0, 0, GetAmbient(N), 0);

		Surface surface;
		surface.init();
		surface.flags |= SURFACE_FLAG_RECEIVE_SHADOW;
		surface.P = P;
		surface.N = N;
		surface.V = 0;
		surface.pixel = pixel;
		surface.sss = 2;
		surface.sss_inv = 1.0 / (1.0 + surface.sss);
		surface.update();

		TiledLighting(surface, lighting, GetFlatTileIndex(pixel));

		color.rgb *= lighting.direct.diffuse + lighting.indirect.diffuse;

		color = max(0, color);
	}
	
	color *= weather.rain_color;

	ApplyFog(dist, V, color);
	
	return color;
}
