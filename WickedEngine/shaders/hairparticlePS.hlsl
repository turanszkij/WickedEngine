#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define SHADOW_MASK_ENABLED
#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"

TEXTURE2D(texture_color, float4, TEXSLOT_ONDEMAND0);

[earlydepthstencil]
GBuffer main(VertexToPixel input)
{
	float4 color = texture_color.Sample(sampler_linear_wrap, input.tex);
	color.rgb = DEGAMMA(color.rgb);
	color.rgb *= input.color;
	float opacity = 1;
	float3 V = g_xCamera_CamPos - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;

	Surface surface;
	surface.create(g_xMaterial, color, 0);
	surface.P = input.pos3D;
	surface.N = input.nor;
	surface.V = V;
	surface.pixel = input.pos.xy;
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

#ifdef SHADOW_MASK_ENABLED
	[branch]
	if (g_xFrame_Options & OPTION_BIT_RAYTRACED_SHADOWS)
	{
		lighting.shadow_mask = texture_rtshadow[surface.pixel];
	}
#endif // SHADOW_MASK_ENABLED

	float depth = input.pos.z;
	float3 reflection = 0;

	TiledLighting(surface, lighting);

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, color);

	return CreateGBuffer(color, surface);
}
