#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "globals.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "lightingHF.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

#include "reconstructPositionHF.hlsli"


struct LightOutputType
{
	float4 diffuse : SV_TARGET0;
	float4 specular: SV_TARGET1;
};


// MACROS

#define DEFERREDLIGHT_MAKEPARAMS														\
	ShaderEntityType light = EntityArray[(uint)g_xColor.x];								\
	Lighting lighting = CreateLighting(0, 0, 0, 0);										\
	float diffuse_alpha = 1;															\
	float specular_alpha = 1;															\
	float2 ScreenCoord = PSIn.pos2D.xy / PSIn.pos2D.w * float2(0.5f, -0.5f) + 0.5f;		\
	float depth = texture_depth[PSIn.pos.xy];											\
	float4 g0 = texture_gbuffer0[PSIn.pos.xy];											\
	float4 g1 = texture_gbuffer1[PSIn.pos.xy];											\
	float4 g2 = texture_gbuffer2[PSIn.pos.xy];											\
	float3 N = decode(g1.xy);															\
	float2 velocity = g1.zw;															\
	float2 ReprojectedScreenCoord = ScreenCoord + velocity;								\
	float3 P = getPosition(ScreenCoord, depth);											\
	float3 V = normalize(g_xCamera_CamPos - P);											\
	Surface surface = CreateSurface(P, N, V, float4(g0.rgb, 1), g2.r, g2.g, g2.b, g2.a);


#define DEFERREDLIGHT_DIRECTIONAL														\
	DirectionalLight(light, surface, lighting);

#define DEFERREDLIGHT_SPOT																\
	SpotLight(light, surface, lighting);

#define DEFERREDLIGHT_POINT																\
	PointLight(light, surface, lighting);

#define DEFERREDLIGHT_SPHERE															\
	SphereLight(light, surface, lighting);

#define DEFERREDLIGHT_DISC																\
	DiscLight(light, surface, lighting);

#define DEFERREDLIGHT_RECTANGLE															\
	RectangleLight(light, surface, lighting);

#define DEFERREDLIGHT_TUBE																\
	TubeLight(light, surface, lighting);


#define DEFERREDLIGHT_RETURN															\
	LightingPart combined_lighting = CombineLighting(surface, lighting);				\
	LightOutputType Out;																\
	Out.diffuse = float4(combined_lighting.diffuse, diffuse_alpha);						\
	Out.specular = float4(combined_lighting.specular, specular_alpha);					\
	return Out;

#endif // _LIGHTHF_



