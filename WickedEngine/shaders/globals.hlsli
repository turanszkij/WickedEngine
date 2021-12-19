#ifndef WI_SHADER_GLOBALS_HF
#define WI_SHADER_GLOBALS_HF
#include "ColorSpaceUtility.hlsli"
#include "PixelPacking_R11G11B10.hlsli"
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

// The root signature will affect shader compilation for DX12.
//	The shader compiler will take the defined name: WICKED_ENGINE_DEFAULT_ROOTSIGNATURE and use it as root signature
//	If you wish to specify custom root signature, make sure that this define is not available
//		(for example: not including this file, or using #undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE)
#define WICKED_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=12, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"DescriptorTable( " \
		"CBV(b2, numDescriptors = 12, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)," \
		"SRV(t0, numDescriptors = 16, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE)," \
		"UAV(u0, numDescriptors = 16, flags = DESCRIPTORS_VOLATILE | DATA_STATIC_WHILE_SET_AT_EXECUTE)" \
	")," \
	"DescriptorTable( " \
		"Sampler(s0, offset = 0, numDescriptors = 8, flags = DESCRIPTORS_VOLATILE)" \
	")," \
	"DescriptorTable( " \
		"Sampler(s0, space = 1, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE)" \
	")," \
	"DescriptorTable( " \
		"SRV(t0, space = 2, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 3, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 4, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 5, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 6, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 7, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 8, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 9, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 10, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 11, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 12, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 13, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 14, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 15, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 16, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 17, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	"), " \
	"StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC)," \
	"StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC)," \
	"StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC)," \
	"StaticSampler(s109, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"


// These are static samplers, they don't need to be bound:
//	They are also on slots that are not bindable as sampler bind slots must be in [0,15] range!
SamplerState sampler_linear_clamp : register(s100);
SamplerState sampler_linear_wrap : register(s101);
SamplerState sampler_linear_mirror : register(s102);
SamplerState sampler_point_clamp : register(s103);
SamplerState sampler_point_wrap : register(s104);
SamplerState sampler_point_mirror : register(s105);
SamplerState sampler_aniso_clamp : register(s106);
SamplerState sampler_aniso_wrap : register(s107);
SamplerState sampler_aniso_mirror : register(s108);
SamplerComparisonState sampler_cmp_depth : register(s109);

SamplerState bindless_samplers[] : register(space1);
Texture2D bindless_textures[] : register(space2);
ByteAddressBuffer bindless_buffers[] : register(space3);
Buffer<uint> bindless_ib[] : register(space4);
#ifdef RTAPI
RaytracingAccelerationStructure bindless_accelerationstructures[] : register(space5);
#endif // RTAPI
Texture2DArray bindless_textures2DArray[] : register(space6);
TextureCube bindless_cubemaps[] : register(space7);
TextureCubeArray bindless_cubearrays[] : register(space8);
Texture3D bindless_textures3D[] : register(space9);
Texture2D<float> bindless_textures_float[] : register(space10);
Texture2D<float2> bindless_textures_float2[] : register(space11);
Texture2D<uint2> bindless_textures_uint2[] : register(space12);
Texture2D<uint4> bindless_textures_uint4[] : register(space13);

RWTexture2D<float4> bindless_rwtextures[] : register(space14);
RWByteAddressBuffer bindless_rwbuffers[] : register(space15);
RWTexture2DArray<float4> bindless_rwtextures2DArray[] : register(space16);
RWTexture3D<float4> bindless_rwtextures3D[] : register(space17);

inline FrameCB GetFrame()
{
	return g_xFrame;
}
inline CameraCB GetCamera()
{
	return g_xCamera;
}
inline ShaderScene GetScene()
{
	return GetFrame().scene;
}
inline ShaderWeather GetWeather()
{
	return GetScene().weather;
}
inline ShaderMeshInstance load_instance(uint instanceIndex)
{
	return bindless_buffers[GetScene().instancebuffer].Load<ShaderMeshInstance>(instanceIndex * sizeof(ShaderMeshInstance));
}
inline ShaderMesh load_mesh(uint meshIndex)
{
	return bindless_buffers[GetScene().meshbuffer].Load<ShaderMesh>(meshIndex * sizeof(ShaderMesh));
}
inline ShaderMeshSubset load_subset(ShaderMesh mesh, uint subsetIndex)
{
	return bindless_buffers[NonUniformResourceIndex(mesh.subsetbuffer)].Load<ShaderMeshSubset>(subsetIndex * sizeof(ShaderMeshSubset));
}
inline ShaderMaterial load_material(uint materialIndex)
{
	return bindless_buffers[GetScene().materialbuffer].Load<ShaderMaterial>(materialIndex * sizeof(ShaderMaterial));
}
inline ShaderEntity load_entity(uint entityIndex)
{
	return bindless_buffers[GetFrame().buffer_entityarray_index].Load<ShaderEntity>(entityIndex * sizeof(ShaderEntity));
}
inline float4x4 load_entitymatrix(uint matrixIndex)
{
	return transpose(bindless_buffers[GetFrame().buffer_entitymatrixarray_index].Load<float4x4>(matrixIndex * sizeof(float4x4)));
}

#define texture_globalenvmap bindless_cubemaps[GetScene().globalenvmap]
#define texture_envmaparray bindless_cubearrays[GetScene().envmaparray]

#define texture_random64x64 bindless_textures[GetFrame().texture_random64x64_index]
#define texture_bluenoise bindless_textures[GetFrame().texture_bluenoise_index]
#define texture_sheenlut bindless_textures[GetFrame().texture_sheenlut_index]
#define texture_skyviewlut bindless_textures[GetFrame().texture_skyviewlut_index]
#define texture_transmittancelut bindless_textures[GetFrame().texture_transmittancelut_index]
#define texture_multiscatteringlut bindless_textures[GetFrame().texture_multiscatteringlut_index]
#define texture_skyluminancelut bindless_textures[GetFrame().texture_skyluminancelut_index]
#define texture_shadowarray_2d bindless_textures2DArray[GetFrame().texture_shadowarray_2d_index]
#define texture_shadowarray_cube bindless_cubearrays[GetFrame().texture_shadowarray_cube_index]
#define texture_shadowarray_transparent_2d bindless_textures2DArray[GetFrame().texture_shadowarray_transparent_2d_index]
#define texture_shadowarray_transparent_cube bindless_cubearrays[GetFrame().texture_shadowarray_transparent_cube_index]
#define texture_voxelgi bindless_textures3D[GetFrame().texture_voxelgi_index]
#define scene_acceleration_structure bindless_accelerationstructures[GetScene().TLAS]

#define texture_depth bindless_textures_float[GetCamera().texture_depth_index]
#define texture_depth_history bindless_textures_float[GetCamera().texture_depth_index_prev]
#define texture_lineardepth bindless_textures_float[GetCamera().texture_lineardepth_index]
#define texture_gbuffer0 bindless_textures_uint2[GetCamera().texture_gbuffer0_index]
#define texture_gbuffer1 bindless_textures_float2[GetCamera().texture_gbuffer1_index]

#define PI 3.14159265358979323846
#define SQRT2 1.41421356237309504880
#define FLT_MAX 3.402823466e+38
#define FLT_EPSILON 1.192092896e-07
#define GOLDEN_RATIO 1.6180339887

#define sqr(a)		((a)*(a))

inline bool is_saturated(float a) { return a == saturate(a); }
inline bool is_saturated(float2 a) { return is_saturated(a.x) && is_saturated(a.y); }
inline bool is_saturated(float3 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z); }
inline bool is_saturated(float4 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z) && is_saturated(a.w); }

#define DEGAMMA_SKY(x)	((GetFrame().options & OPTION_BIT_STATIC_SKY_HDR) ? (x) : RemoveSRGBCurve_Fast(x))
#define DEGAMMA(x)		(RemoveSRGBCurve_Fast(x))
#define GAMMA(x)		(ApplySRGBCurve_Fast(x))

inline float3 GetSunColor() { return GetWeather().sun_color; }
inline float3 GetSunDirection() { return GetWeather().sun_direction; }
inline float GetSunEnergy() { return GetWeather().sun_energy; }
inline float3 GetHorizonColor() { return GetWeather().horizon.rgb; }
inline float3 GetZenithColor() { return GetWeather().zenith.rgb; }
inline float3 GetAmbientColor() { return GetWeather().ambient.rgb; }
inline uint2 GetInternalResolution() { return GetCamera().internal_resolution; }
inline float GetTime() { return GetFrame().time; }
inline uint2 GetTemporalAASampleRotation() { return uint2((GetFrame().temporalaa_samplerotation >> 0u) & 0x000000FF, (GetFrame().temporalaa_samplerotation >> 8) & 0x000000FF); }
inline bool IsStaticSky() { return GetScene().globalenvmap >= 0; }

// Exponential height fog based on: https://www.iquilezles.org/www/articles/fog/fog.htm
// Non constant density function
//	distance	: sample to point distance
//	O			: sample position
//	V			: sample to point vector
inline float GetFogAmount(float distance, float3 O, float3 V)
{
	ShaderFog fog = GetWeather().fog;
	float fogDensity = saturate((distance - fog.start) / (fog.end - fog.start));

	if (GetFrame().options & OPTION_BIT_HEIGHT_FOG)
	{
		float fogFalloffScale = 1.0 / max(0.01, fog.height_end - fog.height_start);

		// solve for x, e^(-h * x) = 0.001
		// x = 6.907755 * h^-1
		float fogFalloff = 6.907755 * fogFalloffScale;
		
		float originHeight = O.y;
		float Z = -V.y;
		float effectiveZ = max(abs(Z), 0.001);

		float endLineHeight = mad(distance, Z, originHeight); // Isolated vector equation for y
		float minLineHeight = min(originHeight, endLineHeight);
		float heightLineFalloff = max(minLineHeight - fog.height_start, 0);
		
		float baseHeightFogDistance = clamp((fog.height_start - minLineHeight) / effectiveZ, 0, distance);
		float exponentialFogDistance = distance - baseHeightFogDistance; // Exclude distance below base height
		float exponentialHeightLineIntegral = exp(-heightLineFalloff * fogFalloff) * (1.0 - exp(-exponentialFogDistance * effectiveZ * fogFalloff)) / (effectiveZ * fogFalloff);
		
		float opticalDepthHeightFog = fogDensity * (baseHeightFogDistance + exponentialHeightLineIntegral);
		float transmittanceHeightFog = exp(-opticalDepthHeightFog);
		
		float fogAmount = transmittanceHeightFog;
		return 1.0 - fogAmount;
	}
	else
	{
		return fogDensity;
	}
}

inline float3 tonemap(float3 x)
{
	return x / (x + 1); // Reinhard tonemap
}
inline float3 inverse_tonemap(float3 x)
{
	return x / (1 - x);
}

inline float4 blue_noise(uint2 pixel)
{
	return frac(texture_bluenoise[pixel % 128].rgba + GetFrame().blue_noise_phase);
}
inline float4 blue_noise(uint2 pixel, float depth)
{
	return frac(texture_bluenoise[pixel % 128].rgba + GetFrame().blue_noise_phase + depth);
}

// Helpers:

// returns a random float in range (0, 1). seed must be >0!
inline float rand(inout float seed, in float2 uv)
{
	float result = frac(sin(seed * dot(uv, float2(12.9898, 78.233))) * 43758.5453);
	seed += 1;
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
	const float2 magicFrameScale = float2(47, 17) * 0.695;
	uv += frameCount * magicFrameScale;

	const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
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
inline float3 vertexID_create_cube(in uint vertexID)
{
	uint b = 1u << vertexID;
	return float3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
}

// Creates a full screen triangle from 3 vertices:
inline void vertexID_create_fullscreen_triangle(in uint vertexID, out float4 pos)
{
	pos.x = (float)(vertexID / 2) * 4.0 - 1.0;
	pos.y = (float)(vertexID % 2) * 4.0 - 1.0;
	pos.z = 0;
	pos.w = 1;
}
inline void vertexID_create_fullscreen_triangle(in uint vertexID, out float4 pos, out float2 uv)
{
	vertexID_create_fullscreen_triangle(vertexID, pos);

	uv.x = (float)(vertexID / 2) * 2;
	uv.y = 1 - (float)(vertexID % 2) * 2;
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
inline float compute_lineardepth(in float z, in float near, in float far)
{
	float z_n = 2 * z - 1;
	float lin = 2 * far * near / (near + far - z_n * (near - far));
	return lin;
}
inline float compute_lineardepth(in float z)
{
	return compute_lineardepth(z, GetCamera().z_near, GetCamera().z_far);
}

inline float3x3 get_tangentspace(in float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99 ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

// Point on hemisphere with uniform distribution
//	u, v : in range [0, 1]
float3 hemispherepoint_uniform(float u, float v) {
	float phi = v * 2 * PI;
	float cosTheta = 1 - u;
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
// Point on hemisphere with cosine-weighted distribution
//	u, v : in range [0, 1]
float3 hemispherepoint_cos(float u, float v) {
	float phi = v * 2 * PI;
	float cosTheta = sqrt(1 - u);
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
// Get random hemisphere sample in world-space along the normal (uniform distribution)
inline float3 sample_hemisphere_uniform(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_uniform(rand(seed, pixel), rand(seed, pixel)), get_tangentspace(normal));
}
// Get random hemisphere sample in world-space along the normal (cosine-weighted distribution)
inline float3 sample_hemisphere_cos(in float3 normal, inout float seed, in float2 pixel)
{
	return mul(hemispherepoint_cos(rand(seed, pixel), rand(seed, pixel)), get_tangentspace(normal));
}

// Reconstructs world-space position from depth buffer
//	uv		: screen space coordinate in [0, 1] range
//	z		: depth value at current pixel
//	InvVP	: Inverse of the View-Projection matrix that was used to generate the depth value
inline float3 reconstruct_position(in float2 uv, in float z, in float4x4 inverse_view_projection)
{
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float4 position_s = float4(x, y, z, 1);
	float4 position_v = mul(inverse_view_projection, position_s);
	return position_v.xyz / position_v.w;
}
inline float3 reconstruct_position(in float2 uv, in float z)
{
	return reconstruct_position(uv, z, GetCamera().inverse_view_projection);
}


// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// uv			: UV texture coordinates on cubemap face in range [0, 1]
// faceIndex	: cubemap face index as in the backing texture2DArray in range [0, 5]
inline float3 uv_to_cubemap(in float2 uv, in uint faceIndex)
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
	float2 texPos1 = floor(samplePos - 0.5) + 0.5;

	// Compute the fractional offset from our starting texel to our original sample location, which we'll
	// feed into the Catmull-Rom spline function to get our filter weights.
	float2 f = samplePos - texPos1;

	// Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
	// These equations are pre-expanded based on our knowledge of where the texels will be located,
	// which lets us avoid having to evaluate a piece-wise function.
	float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
	float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
	float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
	float2 w3 = f * f * (-0.5 + 0.5 * f);

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

	float4 result = 0.0;
	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0) * w0.x * w0.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0) * w12.x * w0.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0) * w3.x * w0.y;

	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0) * w0.x * w12.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0) * w12.x * w12.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0) * w3.x * w12.y;

	result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0) * w0.x * w3.y;
	result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0) * w12.x * w3.y;
	result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0) * w3.x * w3.y;

	return result;
}

inline uint pack_unitvector(in float3 value)
{
	uint retVal = 0;
	retVal |= (uint)((value.x * 0.5 + 0.5) * 255.0) << 0u;
	retVal |= (uint)((value.y * 0.5 + 0.5) * 255.0) << 8u;
	retVal |= (uint)((value.z * 0.5 + 0.5) * 255.0) << 16u;
	return retVal;
}
inline float3 unpack_unitvector(in uint value)
{
	float3 retVal;
	retVal.x = (float)((value >> 0u) & 0xFF) / 255.0 * 2 - 1;
	retVal.y = (float)((value >> 8u) & 0xFF) / 255.0 * 2 - 1;
	retVal.z = (float)((value >> 16u) & 0xFF) / 255.0 * 2 - 1;
	return retVal;
}

inline uint pack_utangent(in float4 value)
{
	uint retVal = 0;
	retVal |= (uint)((value.x * 0.5 + 0.5) * 255.0) << 0u;
	retVal |= (uint)((value.y * 0.5 + 0.5) * 255.0) << 8u;
	retVal |= (uint)((value.z * 0.5 + 0.5) * 255.0) << 16u;
	retVal |= (uint)((value.w * 0.5 + 0.5) * 255.0) << 24u;
	return retVal;
}
inline float4 unpack_utangent(in uint value)
{
	float4 retVal;
	retVal.x = (float)((value >> 0u) & 0xFF) / 255.0;
	retVal.y = (float)((value >> 8u) & 0xFF) / 255.0;
	retVal.z = (float)((value >> 16u) & 0xFF) / 255.0;
	retVal.w = (float)((value >> 24u) & 0xFF) / 255.0;
	return retVal;
}

inline uint pack_rgba(in float4 value)
{
	uint retVal = 0;
	retVal |= (uint)(value.x * 255.0) << 0u;
	retVal |= (uint)(value.y * 255.0) << 8u;
	retVal |= (uint)(value.z * 255.0) << 16u;
	retVal |= (uint)(value.w * 255.0) << 24u;
	return retVal;
}
inline float4 unpack_rgba(in uint value)
{
	float4 retVal;
	retVal.x = (float)((value >> 0u) & 0xFF) / 255.0;
	retVal.y = (float)((value >> 8u) & 0xFF) / 255.0;
	retVal.z = (float)((value >> 16u) & 0xFF) / 255.0;
	retVal.w = (float)((value >> 24u) & 0xFF) / 255.0;
	return retVal;
}

inline uint pack_half2(in float2 value)
{
	uint retVal = 0;
	retVal = f32tof16(value.x) | (f32tof16(value.y) << 16u);
	return retVal;
}
inline float2 unpack_half2(in uint value)
{
	float2 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16u);
	return retVal;
}
inline uint2 pack_half3(in float3 value)
{
	uint2 retVal = 0;
	retVal.x = f32tof16(value.x) | (f32tof16(value.y) << 16u);
	retVal.y = f32tof16(value.z);
	return retVal;
}
inline float3 unpack_half3(in uint2 value)
{
	float3 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16u);
	retVal.z = f16tof32(value.y);
	return retVal;
}
inline uint2 pack_half4(in float4 value)
{
	uint2 retVal = 0;
	retVal.x = f32tof16(value.x) | (f32tof16(value.y) << 16u);
	retVal.y = f32tof16(value.z) | (f32tof16(value.w) << 16u);
	return retVal;
}
inline float4 unpack_half4(in uint2 value)
{
	float4 retVal;
	retVal.x = f16tof32(value.x);
	retVal.y = f16tof32(value.x >> 16u);
	retVal.z = f16tof32(value.y);
	retVal.w = f16tof32(value.y >> 16u);
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
	pos.x = min(max(pos.x * 1024, 0), 1023);
	pos.y = min(max(pos.y * 1024, 0), 1023);
	pos.z = min(max(pos.z * 1024, 0), 1023);
	uint xx = expandBits((uint)pos.x);
	uint yy = expandBits((uint)pos.y);
	uint zz = expandBits((uint)pos.z);
	return xx * 4 + yy * 2 + zz;
}


// Octahedral encodings:
//	Journal of Computer Graphics Techniques Vol. 3, No. 2, 2014 http://jcgt.org

// Returns +/-1
float2 signNotZero(float2 v)
{
	return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}
// Assume normalized input. Output is on [-1, 1] for each component.
float2 encode_oct(in float3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
	float2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}
float3 decode_oct(float2 e)
{
	float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
	return normalize(v);
}

// Assume normalized input on +Z hemisphere.
// Output is on [-1, 1].
float2 encode_hemioct(in float3 v)
{
	// Project the hemisphere onto the hemi-octahedron,
	// and then into the xy plane
	float2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + v.z));
	// Rotate and scale the center diamond to the unit square
	return float2(p.x + p.y, p.x - p.y);
}
float3 decode_hemioct(float2 e)
{
	// Rotate and scale the unit square back to the center diamond
	float2 temp = float2(e.x + e.y, e.x - e.y) * 0.5;
	float3 v = float3(temp, 1.0 - abs(temp.x) - abs(temp.y));
	return normalize(v);
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
float trace_sphere(float3 o, float3 d, float3 center, float radius)
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
float trace_plane(float3 o, float3 d, float3 planeOrigin, float3 planeNormal)
{
	return dot(planeNormal, (planeOrigin - o) / dot(planeNormal, d));
}
// o		: ray origin
// d		: ray direction
// A,B,C	: traingle corners
// returns distance on the ray to the object if hit, 0 otherwise
float trace_triangle(float3 o, float3 d, float3 A, float3 B, float3 C)
{
	float3 planeNormal = normalize(cross(B - A, C - B));
	float t = trace_plane(o, d, A, planeNormal);
	float3 p = o + d * t;

	float3 N1 = normalize(cross(B - A, p - B));
	float3 N2 = normalize(cross(C - B, p - C));
	float3 N3 = normalize(cross(A - C, p - A));

	float d0 = dot(N1, N2);
	float d1 = dot(N2, N3);

	float threshold = 1.0 - 0.001;
	return (d0 > threshold && d1 > threshold) ? 1.0 : 0.0;
}
// o		: ray origin
// d		: ray direction
// A,B,C,D	: rectangle corners
// returns distance on the ray to the object if hit, 0 otherwise
float trace_rectangle(float3 o, float3 d, float3 A, float3 B, float3 C, float3 D)
{
	return max(trace_triangle(o, d, A, B, C), trace_triangle(o, d, C, D, A));
}
// o		: ray origin
// d		: ray direction
// diskNormal : disk facing direction
// returns distance on the ray to the object if hit, 0 otherwise
float trace_disk(float3 o, float3 d, float3 diskCenter, float diskRadius, float3 diskNormal)
{
	float t = trace_plane(o, d, diskCenter, diskNormal);
	float3 p = o + d * t;
	float3 diff = p - diskCenter;
	return dot(diff, diff) < sqr(diskRadius);
}

// Return the closest point on the line (without limit) 
float3 closest_point_on_line(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + t * ab;
}
// Return the closest point on the segment (with limit) 
float3 closest_point_on_segment(float3 a, float3 b, float3 c)
{
	float3 ab = b - a;
	float t = dot(c - a, ab) / dot(ab, ab);
	return a + saturate(t) * ab;
}

// Compute barycentric coordinates on triangle from a point p
float2 compute_barycentrics(float3 p, float3 a, float3 b, float3 c)
{
	float3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom_rcp = rcp(d00 * d11 - d01 * d01);
	float u = (d11 * d20 - d01 * d21) * denom_rcp;
	float v = (d00 * d21 - d01 * d20) * denom_rcp;
	return float2(u, v);
}

static const float4 halton64[] = {
	float4(0.5000000000f, 0.3333333333f, 0.2000000000f, 0.1428571429f),
	float4(0.2500000000f, 0.6666666667f, 0.4000000000f, 0.2857142857f),
	float4(0.7500000000f, 0.1111111111f, 0.6000000000f, 0.4285714286f),
	float4(0.1250000000f, 0.4444444444f, 0.8000000000f, 0.5714285714f),
	float4(0.6250000000f, 0.7777777778f, 0.0400000000f, 0.7142857143f),
	float4(0.3750000000f, 0.2222222222f, 0.2400000000f, 0.8571428571f),
	float4(0.8750000000f, 0.5555555556f, 0.4400000000f, 0.0204081633f),
	float4(0.0625000000f, 0.8888888889f, 0.6400000000f, 0.1632653061f),
	float4(0.5625000000f, 0.0370370370f, 0.8400000000f, 0.3061224490f),
	float4(0.3125000000f, 0.3703703704f, 0.0800000000f, 0.4489795918f),
	float4(0.8125000000f, 0.7037037037f, 0.2800000000f, 0.5918367347f),
	float4(0.1875000000f, 0.1481481481f, 0.4800000000f, 0.7346938776f),
	float4(0.6875000000f, 0.4814814815f, 0.6800000000f, 0.8775510204f),
	float4(0.4375000000f, 0.8148148148f, 0.8800000000f, 0.0408163265f),
	float4(0.9375000000f, 0.2592592593f, 0.1200000000f, 0.1836734694f),
	float4(0.0312500000f, 0.5925925926f, 0.3200000000f, 0.3265306122f),
	float4(0.5312500000f, 0.9259259259f, 0.5200000000f, 0.4693877551f),
	float4(0.2812500000f, 0.0740740741f, 0.7200000000f, 0.6122448980f),
	float4(0.7812500000f, 0.4074074074f, 0.9200000000f, 0.7551020408f),
	float4(0.1562500000f, 0.7407407407f, 0.1600000000f, 0.8979591837f),
	float4(0.6562500000f, 0.1851851852f, 0.3600000000f, 0.0612244898f),
	float4(0.4062500000f, 0.5185185185f, 0.5600000000f, 0.2040816327f),
	float4(0.9062500000f, 0.8518518519f, 0.7600000000f, 0.3469387755f),
	float4(0.0937500000f, 0.2962962963f, 0.9600000000f, 0.4897959184f),
	float4(0.5937500000f, 0.6296296296f, 0.0080000000f, 0.6326530612f),
	float4(0.3437500000f, 0.9629629630f, 0.2080000000f, 0.7755102041f),
	float4(0.8437500000f, 0.0123456790f, 0.4080000000f, 0.9183673469f),
	float4(0.2187500000f, 0.3456790123f, 0.6080000000f, 0.0816326531f),
	float4(0.7187500000f, 0.6790123457f, 0.8080000000f, 0.2244897959f),
	float4(0.4687500000f, 0.1234567901f, 0.0480000000f, 0.3673469388f),
	float4(0.9687500000f, 0.4567901235f, 0.2480000000f, 0.5102040816f),
	float4(0.0156250000f, 0.7901234568f, 0.4480000000f, 0.6530612245f),
	float4(0.5156250000f, 0.2345679012f, 0.6480000000f, 0.7959183673f),
	float4(0.2656250000f, 0.5679012346f, 0.8480000000f, 0.9387755102f),
	float4(0.7656250000f, 0.9012345679f, 0.0880000000f, 0.1020408163f),
	float4(0.1406250000f, 0.0493827160f, 0.2880000000f, 0.2448979592f),
	float4(0.6406250000f, 0.3827160494f, 0.4880000000f, 0.3877551020f),
	float4(0.3906250000f, 0.7160493827f, 0.6880000000f, 0.5306122449f),
	float4(0.8906250000f, 0.1604938272f, 0.8880000000f, 0.6734693878f),
	float4(0.0781250000f, 0.4938271605f, 0.1280000000f, 0.8163265306f),
	float4(0.5781250000f, 0.8271604938f, 0.3280000000f, 0.9591836735f),
	float4(0.3281250000f, 0.2716049383f, 0.5280000000f, 0.1224489796f),
	float4(0.8281250000f, 0.6049382716f, 0.7280000000f, 0.2653061224f),
	float4(0.2031250000f, 0.9382716049f, 0.9280000000f, 0.4081632653f),
	float4(0.7031250000f, 0.0864197531f, 0.1680000000f, 0.5510204082f),
	float4(0.4531250000f, 0.4197530864f, 0.3680000000f, 0.6938775510f),
	float4(0.9531250000f, 0.7530864198f, 0.5680000000f, 0.8367346939f),
	float4(0.0468750000f, 0.1975308642f, 0.7680000000f, 0.9795918367f),
	float4(0.5468750000f, 0.5308641975f, 0.9680000000f, 0.0029154519f),
	float4(0.2968750000f, 0.8641975309f, 0.0160000000f, 0.1457725948f),
	float4(0.7968750000f, 0.3086419753f, 0.2160000000f, 0.2886297376f),
	float4(0.1718750000f, 0.6419753086f, 0.4160000000f, 0.4314868805f),
	float4(0.6718750000f, 0.9753086420f, 0.6160000000f, 0.5743440233f),
	float4(0.4218750000f, 0.0246913580f, 0.8160000000f, 0.7172011662f),
	float4(0.9218750000f, 0.3580246914f, 0.0560000000f, 0.8600583090f),
	float4(0.1093750000f, 0.6913580247f, 0.2560000000f, 0.0233236152f),
	float4(0.6093750000f, 0.1358024691f, 0.4560000000f, 0.1661807580f),
	float4(0.3593750000f, 0.4691358025f, 0.6560000000f, 0.3090379009f),
	float4(0.8593750000f, 0.8024691358f, 0.8560000000f, 0.4518950437f),
	float4(0.2343750000f, 0.2469135802f, 0.0960000000f, 0.5947521866f),
	float4(0.7343750000f, 0.5802469136f, 0.2960000000f, 0.7376093294f),
	float4(0.4843750000f, 0.9135802469f, 0.4960000000f, 0.8804664723f),
	float4(0.9843750000f, 0.0617283951f, 0.6960000000f, 0.0437317784f),
	float4(0.0078125000f, 0.3950617284f, 0.8960000000f, 0.1865889213f),
	float4(0.5078125000f, 0.7283950617f, 0.1360000000f, 0.3294460641f),
};

// This the same as wi::graphics::ColorSpace
enum class ColorSpace
{
	SRGB,			// SDR color space (8 or 10 bits per channel)
	HDR10_ST2084,	// HDR10 color space (10 bits per channel)
	HDR_LINEAR,		// HDR color space (16 bits per channel)
};

#endif // WI_SHADER_GLOBALS_HF
