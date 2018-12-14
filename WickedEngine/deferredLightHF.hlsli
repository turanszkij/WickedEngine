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
	float3 diffuse, specular;															\
	float2 ScreenCoord = PSIn.pos2D.xy / PSIn.pos2D.w * float2(0.5f, -0.5f) + 0.5f;		\
	float depth = texture_depth[PSIn.pos.xy];											\
	float4 g0 = texture_gbuffer0[PSIn.pos.xy];											\
	float4 baseColor = float4(g0.rgb, 1);												\
	float ao = g0.a;																	\
	float4 g1 = texture_gbuffer1[PSIn.pos.xy];											\
	float4 g2 = texture_gbuffer2[PSIn.pos.xy];											\
	float3 N = decode(g1.xy);															\
	float2 velocity = g1.zw;															\
	float roughness = g2.x;																\
	float reflectance = g2.y;															\
	float metalness = g2.z;																\
	float2 ReprojectedScreenCoord = ScreenCoord + velocity;								\
	float3 P = getPosition(ScreenCoord, depth);											\
	float3 V = normalize(g_xCamera_CamPos - P);											\
	Surface surface = CreateSurface(P, N, V, baseColor, roughness, reflectance, metalness);


#define DEFERREDLIGHT_DIRECTIONAL														\
	LightingResult result = DirectionalLight(light, surface);							\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_SPOT																\
	LightingResult result = SpotLight(light, surface);									\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_POINT																\
	LightingResult result = PointLight(light, surface);									\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_SPHERE															\
	LightingResult result = SphereLight(light, surface);								\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_DISC																\
	LightingResult result = DiscLight(light, surface);									\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_RECTANGLE															\
	LightingResult result = RectangleLight(light, surface);								\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_TUBE																\
	LightingResult result = TubeLight(light, surface);									\
	diffuse = result.diffuse;															\
	specular = result.specular;


#define DEFERREDLIGHT_RETURN															\
	LightOutputType Out;																\
	Out.diffuse = float4(diffuse, 1);													\
	Out.specular = float4(specular, 1);													\
	return Out;

#endif // _LIGHTHF_



