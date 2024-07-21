#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(VSOut input) : SV_Target
{
	float3 uv_col = float3(input.uv, input.slice);
	float3 uv_nor = float3(input.uv, input.slice + 1);
	float3 uv_sur = float3(input.uv, input.slice + 2);

	half4 baseColor = impostorTex.Sample(sampler_linear_clamp, uv_col);
	baseColor *= unpack_rgba(input.instanceColor);
	half3 N = impostorTex.Sample(sampler_linear_clamp, uv_nor).rgb * 2 - 1;
	half4 surfaceparams = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	half occlusion = surfaceparams.r;
	half roughness = surfaceparams.g;
	half metalness = surfaceparams.b;
	half reflectance = surfaceparams.a;

	float3 V = GetCamera().position - input.pos3D;
	float dist = length(V);
	V /= dist;

	const uint2 pixel = input.pos.xy; // no longer pixel center!
	const float2 ScreenCoord = input.pos.xy * GetCamera().internal_resolution_rcp; // use pixel center!

	Surface surface;
	surface.init();
	surface.SetReceiveShadow(true);
	surface.roughness = roughness;
	surface.albedo = baseColor.rgb * (1 - max(reflectance, metalness));
	surface.f0 = lerp(reflectance.xxx, baseColor.rgb, metalness);
	surface.occlusion = occlusion;
	surface.P = input.pos3D;
	surface.N = N;
	surface.V = V;
	surface.pixel = input.pos.xy;
	surface.update();

#ifndef PREPASS
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	}
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	TiledLighting(surface, lighting, GetFlatTileIndex(pixel));

	half4 color = 0;

	ApplyLighting(surface, lighting, color);

#ifdef TRANSPARENT
	ApplyAerialPerspective(ScreenCoord, surface.P, color);
#endif // TRANSPARENT
	
	ApplyFog(dist, V, color);
	
	return color;
}
