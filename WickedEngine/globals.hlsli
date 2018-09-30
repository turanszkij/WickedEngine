#ifndef _SHADER_GLOBALS_
#define _SHADER_GLOBALS_
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

TEXTURE2D(texture_depth, float, TEXSLOT_DEPTH)
TEXTURE2D(texture_lineardepth, float, TEXSLOT_LINEARDEPTH)
TEXTURE2D(texture_gbuffer0, float4, TEXSLOT_GBUFFER0)
TEXTURE2D(texture_gbuffer1, float4, TEXSLOT_GBUFFER1)
TEXTURE2D(texture_gbuffer2, float4, TEXSLOT_GBUFFER2)
TEXTURE2D(texture_gbuffer3, float4, TEXSLOT_GBUFFER3)
TEXTURE2D(texture_gbuffer4, float4, TEXSLOT_GBUFFER4)
TEXTURECUBEARRAY(texture_envmaparray, float4, TEXSLOT_ENVMAPARRAY)
TEXTURE2D(texture_decalatlas, float4, TEXSLOT_DECALATLAS)
TEXTURE2DARRAY(texture_shadowarray_2d, float, TEXSLOT_SHADOWARRAY_2D)
TEXTURECUBEARRAY(texture_shadowarray_cube, float, TEXSLOT_SHADOWARRAY_CUBE)
TEXTURE2DARRAY(texture_shadowarray_transparent, float4, TEXSLOT_SHADOWARRAY_TRANSPARENT)
TEXTURE3D(texture_voxelradiance, float4, TEXSLOT_VOXELRADIANCE)

STRUCTUREDBUFFER(EntityIndexList, uint, SBSLOT_ENTITYINDEXLIST);
STRUCTUREDBUFFER(EntityArray, ShaderEntityType, SBSLOT_ENTITYARRAY);
STRUCTUREDBUFFER(MatrixArray, float4x4, SBSLOT_MATRIXARRAY);

TEXTURE2D(texture_0, float4, TEXSLOT_ONDEMAND0)
TEXTURE2D(texture_1, float4, TEXSLOT_ONDEMAND1)
TEXTURE2D(texture_2, float4, TEXSLOT_ONDEMAND2)
TEXTURE2D(texture_3, float4, TEXSLOT_ONDEMAND3)
TEXTURE2D(texture_4, float4, TEXSLOT_ONDEMAND4)
TEXTURE2D(texture_5, float4, TEXSLOT_ONDEMAND5)
TEXTURE2D(texture_6, float4, TEXSLOT_ONDEMAND6)
TEXTURE2D(texture_7, float4, TEXSLOT_ONDEMAND7)
TEXTURE2D(texture_8, float4, TEXSLOT_ONDEMAND8)
TEXTURE2D(texture_9, float4, TEXSLOT_ONDEMAND9)


SAMPLERSTATE(			sampler_linear_clamp,	SSLOT_LINEAR_CLAMP	)
SAMPLERSTATE(			sampler_linear_wrap,	SSLOT_LINEAR_WRAP	)
SAMPLERSTATE(			sampler_linear_mirror,	SSLOT_LINEAR_MIRROR	)
SAMPLERSTATE(			sampler_point_clamp,	SSLOT_POINT_CLAMP	)
SAMPLERSTATE(			sampler_point_wrap,		SSLOT_POINT_WRAP	)
SAMPLERSTATE(			sampler_point_mirror,	SSLOT_POINT_MIRROR	)
SAMPLERSTATE(			sampler_aniso_clamp,	SSLOT_ANISO_CLAMP	)
SAMPLERSTATE(			sampler_aniso_wrap,		SSLOT_ANISO_WRAP	)
SAMPLERSTATE(			sampler_aniso_mirror,	SSLOT_ANISO_MIRROR	)
SAMPLERCOMPARISONSTATE(	sampler_cmp_depth,		SSLOT_CMP_DEPTH		)
SAMPLERSTATE(			sampler_objectshader,	SSLOT_OBJECTSHADER	)

static const float		PI = 3.14159265358979323846;
static const float	 SQRT2 = 1.41421356237309504880;

#define sqr(a)		((a)*(a))

#ifdef DISABLE_ALPHATEST
#define ALPHATEST(x)
#else
#define ALPHATEST(x)	clip((x) - (1.0f - g_xAlphaRef));
#endif

#define DEGAMMA(x)		pow(abs(x),g_xFrame_Gamma)
#define GAMMA(x)		pow(abs(x),1.0/g_xFrame_Gamma)

inline float3 GetSunColor() { return g_xFrame_SunColor; }
inline float3 GetSunDirection() { return g_xFrame_SunDirection; }
inline float3 GetHorizonColor() { return g_xFrame_Horizon.rgb; }
inline float3 GetZenithColor() { return g_xFrame_Zenith.rgb; }
inline float3 GetAmbientColor() { return g_xFrame_Ambient.rgb; }
inline float3 GetAmbient(in float3 N) { return lerp(GetHorizonColor(), GetZenithColor(), saturate(N.y * 0.5f + 0.5f)) + GetAmbientColor(); }
inline float2 GetScreenResolution() { return g_xFrame_ScreenWidthHeight; }
inline float GetScreenWidth() { return g_xFrame_ScreenWidthHeight.x; }
inline float GetScreenHeight() { return g_xFrame_ScreenWidthHeight.y; }
inline float2 GetInternalResolution() { return g_xFrame_InternalResolution; }
inline float GetTime() { return g_xFrame_Time; }
inline float GetEmissive(float emissive) { return emissive * 10.0f; }
inline uint2 GetTemporalAASampleRotation() { return float2((g_xFrame_TemporalAASampleRotation >> 0) & 0x000000FF, (g_xFrame_TemporalAASampleRotation >> 8) & 0x000000FF); }

struct ComputeShaderInput
{
	uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
	uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
	uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
	uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};


// Helpers:

// returns a random float in range (0, 1). seed must not be >0!
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

// 2D array index to flattened 1D array index
inline uint flatten2D(uint2 coord, uint2 dim)
{
	return coord.x + coord.y * dim.x;
}
// flattened array index to 2D array index
inline uint2 unflatten2D(uint idx, uint2 dim)
{
	return uint2(idx % dim.x, idx / dim.x);
}

// 3D array index to flattened 1D array index
inline uint flatten3D(uint3 coord, uint3 dim)
{
	return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
}
// flattened array index to 3D array index
inline uint3 unflatten3D(uint idx, uint3 dim)
{
	const uint z = idx / (dim.x * dim.y);
	idx -= (z * dim.x * dim.y);
	const uint y = idx / dim.x;
	const uint x = idx % dim.x;
	return  uint3(x, y, z);
}

// Creates a unit cube triangle strip from just vertex ID (14 vertices)
inline float3 CreateCube(in uint vertexID)
{
	uint b = 1 << vertexID;
	return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}


// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// uv			: UV texture coordinates on cubemap face in range [0, 1]
// faceIndex	: cubemap face index as in the backing texture2DArray in range [0, 5]
inline float3 UV_to_CubeMap(in float2 uv, in uint faceIndex)
{
	// get uv in [-1, 1] range:
	uv = uv * 2 - 1;

	// and UV.y should point upwards:
	uv.y *= -1;

	switch (faceIndex)
	{
	case 0:
		// +X
		return float3(1, uv.y, -uv.x);
	case 1:
		// -X
		return float3(-1, uv.yx);
	case 2:
		// +Y
		return float3(uv.x, 1, -uv.y);
	case 3:
		// -Y
		return float3(uv.x, -1, uv.y);
	case 4:
		// +Z
		return float3(uv, 1);
	case 5:
		// -Z
		return float3(-uv.x, uv.y, -1);
	default:
		// error
		return 0;
	}
}

#endif // _SHADER_GLOBALS_