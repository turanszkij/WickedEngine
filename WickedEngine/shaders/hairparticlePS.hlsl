#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define SHADOW_MASK_ENABLED
#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"

[earlydepthstencil]
float4 main(VertexToPixel input) : SV_Target
{
	ShaderMaterial material = HairGetMaterial();
	ShaderMeshInstance meshinstance = HairGetInstance();
	
	write_mipmap_feedback(HairGetGeometry().materialIndex, ddx_coarse(input.tex.xyxy), ddy_coarse(input.tex.xyxy));

	half4 color = 1;

	[branch]
	if (material.textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		color = material.textures[BASECOLORMAP].Sample(sampler_linear_wrap, input.tex.xyxy);
	}
	color *= material.GetBaseColor();

	float3 V = input.GetViewVector();
	float dist = length(V);
	V /= dist;
	half emissive = 0;

	const min16uint2 pixel = input.pos.xy; // no longer pixel center!
	const float2 ScreenCoord = input.pos.xy * GetCamera().internal_resolution_rcp; // use pixel center!

	Surface surface;
	surface.init();
	surface.create(material, color, surfacemap_simple);
	surface.P = input.GetPos3D();
	surface.N = input.nor_wet.xyz;
	surface.V = V;
	surface.pixel = input.pos.xy;

#ifndef PREPASS
#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
#ifndef CARTOON
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	}
	[branch]
	if (GetCamera().texture_ssgi_index >= 0)
	{
		surface.ssgi = bindless_textures[GetCamera().texture_ssgi_index].SampleLevel(sampler_linear_clamp, ScreenCoord, 0).rgb;
	}
#endif // CARTOON
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS

	half wet = input.nor_wet.w;
	if(wet > 0)
	{
		surface.albedo = lerp(surface.albedo, 0, wet);
	}

	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	TiledLighting(surface, lighting, GetFlatTileIndex(pixel));
	
	ApplyLighting(surface, lighting, color);

#ifdef TRANSPARENT
	ApplyAerialPerspective(ScreenCoord, surface.P, color);
#endif // TRANSPARENT
	
	ApplyFog(dist, V, color);

	color.rgb = mul(saturationMatrix(material.GetSaturation()), color.rgb);
	
	return color;
}
