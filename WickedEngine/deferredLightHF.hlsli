#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "globals.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "lightingHF.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
	nointerpolation int   lightIndex	: LIGHTARRAYINDEX;
};

#include "reconstructPositionHF.hlsli"

static const float specularMaximumIntensity = 1;


struct LightOutputType
{
	float4 diffuse : SV_TARGET0;
	float4 specular: SV_TARGET1;
};


// MACROS

#define DEFERREDLIGHT_MAKEPARAMS														\
	ShaderEntityType light = EntityArray[PSIn.lightIndex];									\
	float3 diffuse, specular;															\
	float2 screenPos = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;		\
	float depth = texture_depth[PSIn.pos.xy];											\
	float4 baseColor = texture_gbuffer0[PSIn.pos.xy];									\
	float4 g1 = texture_gbuffer1[PSIn.pos.xy];											\
	float4 g3 = texture_gbuffer3[PSIn.pos.xy];											\
	float3 N = decode(g1.xy);															\
	float roughness = g3.x;																\
	float reflectance = g3.y;															\
	float metalness = g3.z;																\
	float ao = g3.w;																	\
	BRDF_HELPER_MAKEINPUTS(baseColor, reflectance, metalness)							\
	float3 P = getPosition(screenPos, depth);											\
	float3 V = normalize(g_xCamera_CamPos - P);


#define DEFERREDLIGHT_DIRECTIONAL														\
	LightingResult result = DirectionalLight(light, N, V, P, roughness, f0);			\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_SPOT																\
	LightingResult result = SpotLight(light, N, V, P, roughness, f0);	\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_POINT																\
	LightingResult result = PointLight(light, N, V, P, roughness, f0);\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_SPHERE															\
	LightingResult result = SphereLight(light, N, V, P, roughness, f0);					\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_DISC																\
	LightingResult result = DiscLight(light, N, V, P, roughness, f0);					\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_RECTANGLE															\
	LightingResult result = RectangleLight(light, N, V, P, roughness, f0);				\
	diffuse = result.diffuse;															\
	specular = result.specular;

#define DEFERREDLIGHT_TUBE																\
	LightingResult result = TubeLight(light, N, V, P, roughness, f0);					\
	diffuse = result.diffuse;															\
	specular = result.specular;


#define DEFERREDLIGHT_RETURN															\
	LightOutputType Out = (LightOutputType)0;											\
	Out.diffuse = float4(diffuse, ao);													\
	Out.specular = float4(specular, 1);													\
	return Out;

#endif // _LIGHTHF_



