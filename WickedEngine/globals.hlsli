#ifndef WI_SHADER_GLOBALS_HF
#define WI_SHADER_GLOBALS_HF
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

TEXTURE2D(texture_depth, float, TEXSLOT_DEPTH);
TEXTURE2D(texture_lineardepth, float, TEXSLOT_LINEARDEPTH);
TEXTURE2D(texture_gbuffer0, float4, TEXSLOT_GBUFFER0);
TEXTURE2D(texture_gbuffer1, float4, TEXSLOT_GBUFFER1);
RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);
TEXTURECUBE(texture_globalenvmap, float4, TEXSLOT_GLOBALENVMAP);
TEXTURE2D(texture_globallightmap, float4, TEXSLOT_GLOBALLIGHTMAP);
TEXTURECUBEARRAY(texture_envmaparray, float4, TEXSLOT_ENVMAPARRAY);
TEXTURE2D(texture_decalatlas, float4, TEXSLOT_DECALATLAS);
TEXTURE2DARRAY(texture_shadowarray_2d, float, TEXSLOT_SHADOWARRAY_2D);
TEXTURECUBEARRAY(texture_shadowarray_cube, float, TEXSLOT_SHADOWARRAY_CUBE);
TEXTURE2DARRAY(texture_shadowarray_transparent, float4, TEXSLOT_SHADOWARRAY_TRANSPARENT);
TEXTURE3D(texture_voxelradiance, float4, TEXSLOT_VOXELRADIANCE);

STRUCTUREDBUFFER(EntityTiles, uint, SBSLOT_ENTITYTILES);
STRUCTUREDBUFFER(EntityArray, ShaderEntity, SBSLOT_ENTITYARRAY);
STRUCTUREDBUFFER(MatrixArray, float4x4, SBSLOT_MATRIXARRAY);

TEXTURE2D(texture_0, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_1, float4, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_2, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(texture_3, float4, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_4, float4, TEXSLOT_ONDEMAND4);
TEXTURE2D(texture_5, float4, TEXSLOT_ONDEMAND5);
TEXTURE2D(texture_6, float4, TEXSLOT_ONDEMAND6);
TEXTURE2D(texture_7, float4, TEXSLOT_ONDEMAND7);
TEXTURE2D(texture_8, float4, TEXSLOT_ONDEMAND8);
TEXTURE2D(texture_9, float4, TEXSLOT_ONDEMAND9);


SAMPLERSTATE(			sampler_linear_clamp,	SSLOT_LINEAR_CLAMP	);
SAMPLERSTATE(			sampler_linear_wrap,	SSLOT_LINEAR_WRAP	);
SAMPLERSTATE(			sampler_linear_mirror,	SSLOT_LINEAR_MIRROR	);
SAMPLERSTATE(			sampler_point_clamp,	SSLOT_POINT_CLAMP	);
SAMPLERSTATE(			sampler_point_wrap,		SSLOT_POINT_WRAP	);
SAMPLERSTATE(			sampler_point_mirror,	SSLOT_POINT_MIRROR	);
SAMPLERSTATE(			sampler_aniso_clamp,	SSLOT_ANISO_CLAMP	);
SAMPLERSTATE(			sampler_aniso_wrap,		SSLOT_ANISO_WRAP	);
SAMPLERSTATE(			sampler_aniso_mirror,	SSLOT_ANISO_MIRROR	);
SAMPLERCOMPARISONSTATE(	sampler_cmp_depth,		SSLOT_CMP_DEPTH		);
SAMPLERSTATE(			sampler_objectshader,	SSLOT_OBJECTSHADER	);

static const float		PI = 3.14159265358979323846;
static const float	 SQRT2 = 1.41421356237309504880;

#define sqr(a)		((a)*(a))

inline bool is_saturated(float a) { return a == saturate(a); }
inline bool is_saturated(float2 a) { return is_saturated(a.x) && is_saturated(a.y); }
inline bool is_saturated(float3 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z); }
inline bool is_saturated(float4 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z) && is_saturated(a.w); }

#ifdef DISABLE_ALPHATEST
#define ALPHATEST(x)
#else
#define ALPHATEST(x)	clip((x) - g_xAlphaRef);
#endif

#define DEGAMMA_SKY(x)	pow(abs(x),g_xFrame_StaticSkyGamma)
#define DEGAMMA(x)		pow(abs(x),g_xFrame_Gamma)
#define GAMMA(x)		pow(abs(x),1.0/g_xFrame_Gamma)

inline float3 GetSunColor() { return g_xFrame_SunColor; }
inline float3 GetSunDirection() { return g_xFrame_SunDirection; }
inline float GetSunEnergy() { return g_xFrame_SunEnergy; }
inline float3 GetHorizonColor() { return g_xFrame_Horizon.rgb; }
inline float3 GetZenithColor() { return g_xFrame_Zenith.rgb; }
inline float3 GetAmbientColor() { return g_xFrame_Ambient.rgb; }
inline float2 GetScreenResolution() { return g_xFrame_ScreenWidthHeight; }
inline float GetScreenWidth() { return g_xFrame_ScreenWidthHeight.x; }
inline float GetScreenHeight() { return g_xFrame_ScreenWidthHeight.y; }
inline float2 GetInternalResolution() { return g_xFrame_InternalResolution; }
inline float GetTime() { return g_xFrame_Time; }
inline uint2 GetTemporalAASampleRotation() { return uint2((g_xFrame_TemporalAASampleRotation >> 0) & 0x000000FF, (g_xFrame_TemporalAASampleRotation >> 8) & 0x000000FF); }
inline bool IsStaticSky() { return g_xFrame_StaticSkyGamma > 0.0f; }
inline void ConvertToSpecularGlossiness(inout float4 surface_occlusion_roughness_metallic_reflectance)
{
	surface_occlusion_roughness_metallic_reflectance.r = 1;
	surface_occlusion_roughness_metallic_reflectance.g = 1 - surface_occlusion_roughness_metallic_reflectance.a;
	surface_occlusion_roughness_metallic_reflectance.b = max(surface_occlusion_roughness_metallic_reflectance.r, max(surface_occlusion_roughness_metallic_reflectance.g, surface_occlusion_roughness_metallic_reflectance.b));
	surface_occlusion_roughness_metallic_reflectance.a = 0.02f;
}

inline float GetFogAmount(float dist)
{
	return saturate((dist - g_xFrame_Fog.x) / (g_xFrame_Fog.y - g_xFrame_Fog.x));
}

float3 tonemap(float3 x)
{
	return x / (x + 1); // Reinhard tonemap
}
float3 inverseTonemap(float3 x)
{
	return x / (1 - x);
}


// Helpers:

// returns a random float in range (0, 1). seed must be >0!
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

// A uniform 2D random generator for hemisphere sampling: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
//	idx	: iteration index
//	num	: number of iterations in total
inline float2 hammersley2d(uint idx, uint num) {
	uint bits = idx;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float radicalInverse_VdC = float(bits) * 2.3283064365386963e-10; // / 0x100000000

	return float2(float(idx) / float(num), radicalInverse_VdC);
}

// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// http://advances.realtimerendering.com/s2014/index.html
float InterleavedGradientNoise(float2 uv, uint frameCount)
{
	const float2 magicFrameScale = float2(47, 17) * 0.695f;
	uv += frameCount * magicFrameScale;

	const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
	return frac(magic.z * frac(dot(uv, magic.xy)));
}

// https://www.shadertoy.com/view/llGSzw
float hash1(uint n)
{
	// integer hash copied from Hugo Elias
	n = (n << 13U) ^ n;
	n = n * (n * n * 15731U + 789221U) + 1376312589U;
	return float(n & 0x7fffffffU) / float(0x7fffffff);
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

// Creates a full screen triangle from 3 vertices:
inline void FullScreenTriangle(in uint vertexID, out float4 pos)
{
	pos.x = (float)(vertexID / 2) * 4.0f - 1.0f;
	pos.y = (float)(vertexID % 2) * 4.0f - 1.0f;
	pos.z = 0.0f;
	pos.w = 1.0f;
}
inline void FullScreenTriangle(in uint vertexID, out float4 pos, out float2 uv)
{
	FullScreenTriangle(vertexID, pos);

	uv.x = (float)(vertexID / 2) * 2.0f;
	uv.y = 1.0f - (float)(vertexID % 2) * 2.0f;
}

// Computes a tangent-basis matrix for a surface using screen-space derivatives
inline float3x3 compute_tangent_frame(float3 N, float3 P, float2 UV)
{
#if 1
	// http://www.thetenthplanet.de/archives/1180
	// get edge vectors of the pixel triangle
	float3 dp1 = ddx_coarse(P);
	float3 dp2 = ddy_coarse(P);
	float2 duv1 = ddx_coarse(UV);
	float2 duv2 = ddy_coarse(UV);

	// solve the linear system
	float3 dp2perp = cross(dp2, N);
	float3 dp1perp = cross(N, dp1);
	float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame 
	float invmax = rcp(sqrt(max(dot(T, T), dot(B, B))));
	return float3x3(T * invmax, B * invmax, N);
#else
	// Old version
	float3 dp1 = ddx_coarse(P);
	float3 dp2 = ddy_coarse(P);
	float2 duv1 = ddx_coarse(UV);
	float2 duv2 = ddy_coarse(UV);

	float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
	float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
	float3 T = normalize(mul(float2(duv1.x, duv2.x), inverseM));
	float3 B = normalize(mul(float2(duv1.y, duv2.y), inverseM));

	return float3x3(T, B, N);
#endif
}

// Computes linear depth from post-projection depth
inline float getLinearDepth(in float z, in float near, in float far)
{
	float z_n = 2.0 * z - 1.0;
	float lin = 2.0 * far * near / (near + far - z_n * (near - far));
	return lin;
}
inline float getLinearDepth(in float z)
{
	return getLinearDepth(z, g_xCamera_ZNearP, g_xCamera_ZFarP);
}

inline float3x3 GetTangentSpace(in float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99f ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

// Point on hemisphere with uniform distribution
//	u, v : in range [0, 1]
float3 hemispherepoint_uniform(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = 1.0 - u;
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
// Point on hemisphere with cosine-weighted distribution
//	u, v : in range [0, 1]
float3 hemispherepoint_cos(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
// Get random hemisphere sample in world-space along the normal (uniform distribution)
inline float3 SampleHemisphere_uniform(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_uniform(rand(seed, pixel), rand(seed, pixel)), GetTangentSpace(normal));
}
// Get random hemisphere sample in world-space along the normal (cosine-weighted distribution)
inline float3 SampleHemisphere_cos(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_cos(rand(seed, pixel), rand(seed, pixel)), GetTangentSpace(normal));
}

// Reconstructs world-space position from depth buffer
//	uv		: screen space coordinate in [0, 1] range
//	z		: depth value at current pixel
//	InvVP	: Inverse of the View-Projection matrix that was used to generate the depth value
inline float3 reconstructPosition(in float2 uv, in float z, in float4x4 InvVP)
{
	float x = uv.x * 2.0f - 1.0f;
	float y = (1.0 - uv.y) * 2.0f - 1.0f;
	float4 position_s = float4(x, y, z, 1.0f);
	float4 position_v = mul(InvVP, position_s);
	return position_v.xyz / position_v.w;
}
inline float3 reconstructPosition(in float2 uv, in float z)
{
	return reconstructPosition(uv, z, g_xCamera_InvVP);
}


// For storing normal in gbuffer in world space, we convert to spherical coordinates:
//  [Commented out optimized parts, because we don't need to normalize range when storing to float format]
inline float2 encodeNormal(in float3 N)
{
	return float2(atan2(N.y, N.x) /*/ PI*/, N.z)/* * 0.5f + 0.5f*/;
}
inline float3 decodeNormal(in float2 spherical)
{
	float2 sinCosTheta, sinCosPhi;
	//spherical = spherical * 2.0f - 1.0f;
	sincos(spherical.x /** PI*/, sinCosTheta.x, sinCosTheta.y);
	sinCosPhi = float2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);
	return float3(sinCosTheta.y * sinCosPhi.x, sinCosTheta.x * sinCosPhi.x, sinCosPhi.y);
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

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16. ( https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1#file-tex2dcatmullrom-hlsl )
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float4 SampleTextureCatmullRom(in Texture2D<float4> tex, in SamplerState linearSampler, in float2 uv)
{
	float2 texSize;
	tex.GetDimensions(texSize.x, texSize.y);

	// We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
	// down the sample location to get the exact center of our "starting" texel. The starting texel will be at
	// location [1, 1] in the grid, where [0, 0] is the top left corner.
	float2 samplePos = uv * texSize;
	float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

	// Compute the fractional offset from our starting texel to our original sample location, which we'll
	// feed into the Catmull-Rom spline function to get our filter weights.
	float2 f = samplePos - texPos1;

	// Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
	// These equations are pre-expanded based on our knowledge of where the texels will be located,
	// which lets us avoid having to evaluate a piece-wise function.
	float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
	float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
	float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
	float2 w3 = f * f * (-0.5f + 0.5f * f);

	// Work out weighting factors and sampling offsets that will let us use bilinear filtering to
	// simultaneously evaluate the middle 2 samples from the 4x4 grid.
	float2 w12 = w1 + w2;
	float2 offset12 = w2 / (w1 + w2);

	// Compute the final UV coordinates we'll use for sampling the texture
	float2 texPos0 = texPos1 - 1;
	float2 texPos3 = texPos1 + 2;
	float2 texPos12 = texPos1 + offset12;

	texPos0 /= texSize;
	texPos3 /= texSize;
	texPos12 /= texSize;

	float4 result = 0.0f;
	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

	return result;
}

inline uint pack_unitvector(in float3 value)
{
	uint retVal = 0;
	retVal |= (uint)((value.x * 0.5f + 0.5f) * 255.0f) << 0;
	retVal |= (uint)((value.y * 0.5f + 0.5f) * 255.0f) << 8;
	retVal |= (uint)((value.z * 0.5f + 0.5f) * 255.0f) << 16;
	return retVal;
}
inline float3 unpack_unitvector(in uint value)
{
	float3 retVal;
	retVal.x = (float)((value >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	retVal.y = (float)((value >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	retVal.z = (float)((value >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	return retVal;
}

inline uint pack_rgba(in float4 value)
{
	uint retVal = 0;
	retVal |= (uint)(value.x * 255.0f) << 0;
	retVal |= (uint)(value.y * 255.0f) << 8;
	retVal |= (uint)(value.z * 255.0f) << 16;
	retVal |= (uint)(value.w * 255.0f) << 24;
	return retVal;
}
inline float4 unpack_rgba(in uint value)
{
	float4 retVal;
	retVal.x = (float)((value >> 0) & 0x000000FF) / 255.0f;
	retVal.y = (float)((value >> 8) & 0x000000FF) / 255.0f;
	retVal.z = (float)((value >> 16) & 0x000000FF) / 255.0f;
	retVal.w = (float)((value >> 24) & 0x000000FF) / 255.0f;
	return retVal;
}

inline uint2 pack_half2(in float2 value)
{
	uint retVal = 0;
	retVal = f32tof16(value.x) | (f32tof16(value.y) << 16);
	return retVal;
}
inline float2 unpack_half2(in uint value)
{
	float2 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16);
	return retVal;
}
inline uint2 pack_half3(in float3 value)
{
	uint2 retVal = 0;
	retVal.x = f32tof16(value.x) | (f32tof16(value.y) << 16);
	retVal.y = f32tof16(value.z);
	return retVal;
}
inline float3 unpack_half3(in uint2 value)
{
	float3 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16);
	retVal.z = f16tof32(value.y);
	return retVal;
}
inline uint2 pack_half4(in float4 value)
{
	uint2 retVal = 0;
	retVal.x = f32tof16(value.x) | (f32tof16(value.y) << 16);
	retVal.y = f32tof16(value.z) | (f32tof16(value.w) << 16);
	return retVal;
}
inline float4 unpack_half4(in uint2 value)
{
	float4 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16);
	retVal.z = f16tof32(value.y);
	retVal.w = f16tof32(value.y >> 16);
	return retVal;
}


// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
inline uint expandBits(uint v)
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
inline uint morton3D(in float3 pos)
{
	pos.x = min(max(pos.x * 1024.0f, 0.0f), 1023.0f);
	pos.y = min(max(pos.y * 1024.0f, 0.0f), 1023.0f);
	pos.z = min(max(pos.z * 1024.0f, 0.0f), 1023.0f);
	uint xx = expandBits((uint)pos.x);
	uint yy = expandBits((uint)pos.y);
	uint zz = expandBits((uint)pos.z);
	return xx * 4 + yy * 2 + zz;
}



static const float2x2 BayerMatrix2 =
{
	1.0 / 5.0, 3.0 / 5.0,
	4.0 / 5.0, 2.0 / 5.0
};

static const float3x3 BayerMatrix3 =
{
	3.0 / 10.0, 7.0 / 10.0, 4.0 / 10.0,
	6.0 / 10.0, 1.0 / 10.0, 9.0 / 10.0,
	2.0 / 10.0, 8.0 / 10.0, 5.0 / 10.0
};

static const float4x4 BayerMatrix4 =
{
	1.0 / 17.0, 9.0 / 17.0, 3.0 / 17.0, 11.0 / 17.0,
	13.0 / 17.0, 5.0 / 17.0, 15.0 / 17.0, 7.0 / 17.0,
	4.0 / 17.0, 12.0 / 17.0, 2.0 / 17.0, 10.0 / 17.0,
	16.0 / 17.0, 8.0 / 17.0, 14.0 / 17.0, 6.0 / 17.0
};

static const float BayerMatrix8[8][8] =
{
	{ 1.0 / 65.0, 49.0 / 65.0, 13.0 / 65.0, 61.0 / 65.0, 4.0 / 65.0, 52.0 / 65.0, 16.0 / 65.0, 64.0 / 65.0 },
	{ 33.0 / 65.0, 17.0 / 65.0, 45.0 / 65.0, 29.0 / 65.0, 36.0 / 65.0, 20.0 / 65.0, 48.0 / 65.0, 32.0 / 65.0 },
	{ 9.0 / 65.0, 57.0 / 65.0, 5.0 / 65.0, 53.0 / 65.0, 12.0 / 65.0, 60.0 / 65.0, 8.0 / 65.0, 56.0 / 65.0 },
	{ 41.0 / 65.0, 25.0 / 65.0, 37.0 / 65.0, 21.0 / 65.0, 44.0 / 65.0, 28.0 / 65.0, 40.0 / 65.0, 24.0 / 65.0 },
	{ 3.0 / 65.0, 51.0 / 65.0, 15.0 / 65.0, 63.0 / 65.0, 2.0 / 65.0, 50.0 / 65.0, 14.0 / 65.0, 62.0 / 65.0 },
	{ 35.0 / 65.0, 19.0 / 65.0, 47.0 / 65.0, 31.0 / 65.0, 34.0 / 65.0, 18.0 / 65.0, 46.0 / 65.0, 30.0 / 65.0 },
	{ 11.0 / 65.0, 59.0 / 65.0, 7.0 / 65.0, 55.0 / 65.0, 10.0 / 65.0, 58.0 / 65.0, 6.0 / 65.0, 54.0 / 65.0 },
	{ 43.0 / 65.0, 27.0 / 65.0, 39.0 / 65.0, 23.0 / 65.0, 42.0 / 65.0, 26.0 / 65.0, 38.0 / 65.0, 22.0 / 65.0 }
};


inline float ditherMask2(in float2 pixel)
{
	return BayerMatrix2[pixel.x % 2][pixel.y % 2];
}

inline float ditherMask3(in float2 pixel)
{
	return BayerMatrix3[pixel.x % 3][pixel.y % 3];
}

inline float ditherMask4(in float2 pixel)
{
	return BayerMatrix4[pixel.x % 4][pixel.y % 4];
}

inline float ditherMask8(in float2 pixel)
{
	return BayerMatrix8[pixel.x % 8][pixel.y % 8];
}

inline float dither(in float2 pixel)
{
	return ditherMask8(pixel);
}

// o		: ray origin
// d		: ray direction
// center	: sphere center
// radius	: sphere radius
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_sphere(float3 o, float3 d, float3 center, float radius)
{
	float3 rc = o - center;
	float c = dot(rc, rc) - (radius * radius);
	float b = dot(d, rc);
	float dd = b * b - c;
	float t = -b - sqrt(abs(dd));
	float st = step(0.0, min(t, dd));
	return lerp(-1.0, t, st);
}
// o		: ray origin
// d		: ray direction
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_plane(float3 o, float3 d, float3 planeOrigin, float3 planeNormal)
{
	return dot(planeNormal, (planeOrigin - o) / dot(planeNormal, d));
}
// o		: ray origin
// d		: ray direction
// A,B,C	: traingle corners
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_triangle(float3 o, float3 d, float3 A, float3 B, float3 C)
{
	float3 planeNormal = normalize(cross(B - A, C - B));
	float t = Trace_plane(o, d, A, planeNormal);
	float3 p = o + d * t;

	float3 N1 = normalize(cross(B - A, p - B));
	float3 N2 = normalize(cross(C - B, p - C));
	float3 N3 = normalize(cross(A - C, p - A));

	float d0 = dot(N1, N2);
	float d1 = dot(N2, N3);

	float threshold = 1.0f - 0.001f;
	return (d0 > threshold && d1 > threshold) ? 1.0f : 0.0f;
}
// o		: ray origin
// d		: ray direction
// A,B,C,D	: rectangle corners
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_rectangle(float3 o, float3 d, float3 A, float3 B, float3 C, float3 D)
{
	return max(Trace_triangle(o, d, A, B, C), Trace_triangle(o, d, C, D, A));
}
// o		: ray origin
// d		: ray direction
// diskNormal : disk facing direction
// returns distance on the ray to the object if hit, 0 otherwise
float Trace_disk(float3 o, float3 d, float3 diskCenter, float diskRadius, float3 diskNormal)
{
	float t = Trace_plane(o, d, diskCenter, diskNormal);
	float3 p = o + d * t;
	float3 diff = p - diskCenter;
	return dot(diff, diff) < sqr(diskRadius);
}

// Return the closest point on the line (without limit) 
float3 ClosestPointOnLine(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + t * ab;
}
// Return the closest point on the segment (with limit) 
float3 ClosestPointOnSegment(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + saturate(t) * ab;
}

#endif // WI_SHADER_GLOBALS_HF
