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

	float4 color = 1;

	[branch]
	if (material.textures[BASECOLORMAP].IsValid() && (GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		color = material.textures[BASECOLORMAP].Sample(sampler_linear_wrap, input.tex.xyxy);
	}
	color *= material.baseColor;

	float3 V = GetCamera().position - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;

	const uint2 pixel = input.pos.xy;
	const float2 ScreenCoord = pixel * GetCamera().internal_resolution_rcp;

	Surface surface;
	surface.init();
	surface.create(material, color, surfacemap_simple);
	surface.P = input.pos3D;
	surface.N = input.nor;
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
#endif // CARTOON
#endif // TRANSPARENT
#endif // ENVMAPRENDERING
#endif // PREPASS

	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	float depth = input.pos.z;
	float3 reflection = 0;

	TiledLighting(surface, lighting, GetFlatTileIndex(pixel));


	ApplyLighting(surface, lighting, color);

#ifdef TRANSPARENT
	ApplyAerialPerspective(ScreenCoord, surface.P, color);
#endif // TRANSPARENT
	
	ApplyFog(dist, V, color);
	
	return color;
}
