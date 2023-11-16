#ifndef WI_SHADER_GLOBALS_HF
#define WI_SHADER_GLOBALS_HF
#include "ColorSpaceUtility.hlsli"
#include "PixelPacking_R11G11B10.hlsli"
#include "PixelPacking_RGBE.hlsli"
#include "ShaderInterop.h"

// The root signature will affect shader compilation for DX12.
//	The shader compiler will take the defined name: WICKED_ENGINE_DEFAULT_ROOTSIGNATURE and use it as root signature
//	If you wish to specify custom root signature, make sure that this define is not available
//		(for example: not including this file, or using #undef WICKED_ENGINE_DEFAULT_ROOTSIGNATURE)
#define WICKED_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=12, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"CBV(b2), " \
	"DescriptorTable( " \
		"CBV(b3, numDescriptors = 11, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)," \
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
		"SRV(t0, space = 14, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 15, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 16, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 17, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 18, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 19, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 20, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 21, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 22, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 23, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 24, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 25, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 26, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 27, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 28, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 29, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 30, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 31, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 32, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"UAV(u0, space = 33, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 34, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 35, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 36, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 37, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 38, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 39, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 40, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	"), " \
	"StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
	"StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
	"StaticSampler(s109, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"

#ifndef __PSSL__
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
#endif // __PSSL__

#if defined(__PSSL__)
static const BindlessResource<SamplerState> bindless_samplers;
static const BindlessResource<Texture2D> bindless_textures;
static const BindlessResource<ByteAddressBuffer> bindless_buffers;
static const BindlessResource<Buffer<uint>> bindless_buffers_uint;
static const BindlessResource<Buffer<uint2>> bindless_buffers_uint2;
static const BindlessResource<Buffer<uint3>> bindless_buffers_uint3;
static const BindlessResource<Buffer<uint4>> bindless_buffers_uint4;
static const BindlessResource<Buffer<float>> bindless_buffers_float;
static const BindlessResource<Buffer<float2>> bindless_buffers_float2;
static const BindlessResource<Buffer<float3>> bindless_buffers_float3;
static const BindlessResource<Buffer<float4>> bindless_buffers_float4;
static const BindlessResource<Texture2DArray> bindless_textures2DArray;
static const BindlessResource<TextureCube> bindless_cubemaps;
static const BindlessResource<TextureCubeArray> bindless_cubearrays;
static const BindlessResource<Texture3D> bindless_textures3D;
static const BindlessResource<Texture2D<float>> bindless_textures_float;
static const BindlessResource<Texture2D<float2>> bindless_textures_float2;
static const BindlessResource<Texture2D<uint>> bindless_textures_uint;
static const BindlessResource<Texture2D<uint4>> bindless_textures_uint4;

static const BindlessResource<RWTexture2D<float4>> bindless_rwtextures;
static const BindlessResource<RWByteAddressBuffer> bindless_rwbuffers;
static const BindlessResource<RWBuffer<uint>> bindless_rwbuffers_uint;
static const BindlessResource<RWBuffer<uint2>> bindless_rwbuffers_uint2;
static const BindlessResource<RWBuffer<uint3>> bindless_rwbuffers_uint3;
static const BindlessResource<RWBuffer<uint4>> bindless_rwbuffers_uint4;
static const BindlessResource<RWBuffer<float>> bindless_rwbuffers_float;
static const BindlessResource<RWBuffer<float2>> bindless_rwbuffers_float2;
static const BindlessResource<RWBuffer<float3>> bindless_rwbuffers_float3;
static const BindlessResource<RWBuffer<float4>> bindless_rwbuffers_float4;
static const BindlessResource<RWTexture2DArray<float4>> bindless_rwtextures2DArray;
static const BindlessResource<RWTexture3D<float4>> bindless_rwtextures3D;
static const BindlessResource<RWTexture2D<uint>> bindless_rwtextures_uint;

#elif defined(__spirv__)
// In Vulkan, we can manually overlap descriptor sets to reduce bindings:
//	Note that HLSL register space declaration was not working correctly with overlapped spaces,
//	But vk::binding works correctly in this case.
//	HLSL register space declaration is working well with Vulkan when spaces are not overlapping.
static const uint DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER = 1;
static const uint DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER = 2;
static const uint DESCRIPTOR_SET_BINDLESS_SAMPLER = 3;
static const uint DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE = 4;
static const uint DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE = 5;
static const uint DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER = 6;
static const uint DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE = 7;

[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] ByteAddressBuffer bindless_buffers[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<uint> bindless_buffers_uint[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<uint2> bindless_buffers_uint2[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<uint3> bindless_buffers_uint3[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<uint4> bindless_buffers_uint4[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<float> bindless_buffers_float[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<float2> bindless_buffers_float2[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<float3> bindless_buffers_float3[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER)]] Buffer<float4> bindless_buffers_float4[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLER)]] SamplerState bindless_samplers[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2D bindless_textures[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2DArray bindless_textures2DArray[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] TextureCube bindless_cubemaps[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] TextureCubeArray bindless_cubearrays[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture3D bindless_textures3D[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2D<float> bindless_textures_float[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2D<float2> bindless_textures_float2[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2D<uint> bindless_textures_uint[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE)]] Texture2D<uint4> bindless_textures_uint4[];

[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] RWByteAddressBuffer bindless_rwbuffers[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<uint> bindless_rwbuffers_uint[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<uint2> bindless_rwbuffers_uint2[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<uint3> bindless_rwbuffers_uint3[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<uint4> bindless_rwbuffers_uint4[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<float> bindless_rwbuffers_float[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<float2> bindless_rwbuffers_float2[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<float3> bindless_rwbuffers_float3[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER)]] RWBuffer<float4> bindless_rwbuffers_float4[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE)]] RWTexture2D<float4> bindless_rwtextures[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE)]] RWTexture2DArray<float4> bindless_rwtextures2DArray[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE)]] RWTexture3D<float4> bindless_rwtextures3D[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE)]] RWTexture2D<uint> bindless_rwtextures_uint[];
#ifdef RTAPI
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE)]] RaytracingAccelerationStructure bindless_accelerationstructures[];
#endif // RTAPI

#else
SamplerState bindless_samplers[] : register(space1);
Texture2D bindless_textures[] : register(space2);
ByteAddressBuffer bindless_buffers[] : register(space3);
Buffer<uint> bindless_buffers_uint[] : register(space4);
Buffer<uint2> bindless_buffers_uint2[] : register(space5);
Buffer<uint3> bindless_buffers_uint3[] : register(space6);
Buffer<uint4> bindless_buffers_uint4[] : register(space7);
Buffer<float> bindless_buffers_float[] : register(space8);
Buffer<float2> bindless_buffers_float2[] : register(space9);
Buffer<float3> bindless_buffers_float3[] : register(space10);
Buffer<float4> bindless_buffers_float4[] : register(space11);
#ifdef RTAPI
RaytracingAccelerationStructure bindless_accelerationstructures[] : register(space12);
#endif // RTAPI
Texture2DArray bindless_textures2DArray[] : register(space13);
TextureCube bindless_cubemaps[] : register(space14);
TextureCubeArray bindless_cubearrays[] : register(space15);
Texture3D bindless_textures3D[] : register(space16);
Texture2D<float> bindless_textures_float[] : register(space17);
Texture2D<float2> bindless_textures_float2[] : register(space18);
Texture2D<uint> bindless_textures_uint[] : register(space19);
Texture2D<uint4> bindless_textures_uint4[] : register(space20);

RWTexture2D<float4> bindless_rwtextures[] : register(space21);
RWByteAddressBuffer bindless_rwbuffers[] : register(space22);
RWBuffer<uint> bindless_rwbuffers_uint[] : register(space23);
RWBuffer<uint2> bindless_rwbuffers_uint2[] : register(space24);
RWBuffer<uint3> bindless_rwbuffers_uint3[] : register(space25);
RWBuffer<uint4> bindless_rwbuffers_uint4[] : register(space26);
RWBuffer<float> bindless_rwbuffers_float[] : register(space27);
RWBuffer<float2> bindless_rwbuffers_float2[] : register(space28);
RWBuffer<float3> bindless_rwbuffers_float3[] : register(space29);
RWBuffer<float4> bindless_rwbuffers_float4[] : register(space30);
RWTexture2DArray<float4> bindless_rwtextures2DArray[] : register(space31);
RWTexture3D<float4> bindless_rwtextures3D[] : register(space32);
RWTexture2D<uint> bindless_rwtextures_uint[] : register(space33);

#endif // __spirv__

#include "ShaderInterop_Renderer.h"

#if defined(__PSSL__)
static const BindlessResource<StructuredBuffer<ShaderMeshInstance>> bindless_structured_meshinstance;
static const BindlessResource<StructuredBuffer<ShaderGeometry>> bindless_structured_geometry;
static const BindlessResource<StructuredBuffer<ShaderMeshlet>> bindless_structured_meshlet;
static const BindlessResource<StructuredBuffer<ShaderMaterial>> bindless_structured_material;
static const BindlessResource<StructuredBuffer<ShaderEntity>> bindless_structured_entity;
static const BindlessResource<StructuredBuffer<float4x4>> bindless_structured_matrix;
static const BindlessResource<StructuredBuffer<uint>> bindless_structured_uint;
#elif defined(__spirv__)
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<ShaderMeshInstance> bindless_structured_meshinstance[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<ShaderGeometry> bindless_structured_geometry[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<ShaderMeshlet> bindless_structured_meshlet[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<ShaderMaterial> bindless_structured_material[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<ShaderEntity> bindless_structured_entity[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<float4x4> bindless_structured_matrix[];
[[vk::binding(0, DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER)]] StructuredBuffer<uint> bindless_structured_uint[];
#else
StructuredBuffer<ShaderMeshInstance> bindless_structured_meshinstance[] : register(space34);
StructuredBuffer<ShaderGeometry> bindless_structured_geometry[] : register(space35);
StructuredBuffer<ShaderMeshlet> bindless_structured_meshlet[] : register(space36);
StructuredBuffer<ShaderMaterial> bindless_structured_material[] : register(space37);
StructuredBuffer<ShaderEntity> bindless_structured_entity[] : register(space38);
StructuredBuffer<float4x4> bindless_structured_matrix[] : register(space39);
StructuredBuffer<uint> bindless_structured_uint[] : register(space40);
#endif // __spirv__

inline FrameCB GetFrame()
{
	return g_xFrame;
}
inline ShaderCamera GetCamera(uint camera_index = 0)
{
	return g_xCamera.cameras[camera_index];
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
	return bindless_structured_meshinstance[GetScene().instancebuffer][instanceIndex];
}
inline ShaderGeometry load_geometry(uint geometryIndex)
{
	return bindless_structured_geometry[GetScene().geometrybuffer][geometryIndex];
}
inline ShaderMeshlet load_meshlet(uint meshletIndex)
{
	return bindless_structured_meshlet[GetScene().meshletbuffer][meshletIndex];
}
inline ShaderMaterial load_material(uint materialIndex)
{
	return bindless_structured_material[GetScene().materialbuffer][materialIndex];
}
uint load_entitytile(uint tileIndex)
{
#ifdef TRANSPARENT
	return bindless_structured_uint[GetCamera().buffer_entitytiles_transparent_index][tileIndex];
#else
	return bindless_structured_uint[GetCamera().buffer_entitytiles_opaque_index][tileIndex];
#endif // TRANSPARENT
}
inline ShaderEntity load_entity(uint entityIndex)
{
	return bindless_structured_entity[GetFrame().buffer_entity_index][entityIndex];
}
inline float4x4 load_entitymatrix(uint matrixIndex)
{
	return bindless_structured_matrix[GetFrame().buffer_entity_index][SHADER_ENTITY_COUNT + matrixIndex];
}

struct PrimitiveID
{
	uint primitiveIndex;
	uint instanceIndex;
	uint subsetIndex;

	// These packing methods require meshlet data, and pack into 32 bits:
	inline uint pack()
	{
		// 24 bit meshletIndex
		// 8  bit meshletPrimitiveIndex
		ShaderMeshInstance inst = load_instance(instanceIndex);
		ShaderGeometry geometry = load_geometry(inst.geometryOffset + subsetIndex);
		uint meshletIndex = inst.meshletOffset + geometry.meshletOffset + primitiveIndex / MESHLET_TRIANGLE_COUNT;
		meshletIndex += 1; // indicate that it is valid
		meshletIndex &= ~0u >> 8u; // mask 24 active bits
		uint meshletPrimitiveIndex = primitiveIndex % MESHLET_TRIANGLE_COUNT;
		meshletPrimitiveIndex &= 0xFF; // mask 8 active bits
		meshletPrimitiveIndex <<= 24u;
		return meshletPrimitiveIndex | meshletIndex;
	}
	inline void unpack(uint value)
	{
		uint meshletIndex = value & (~0u >> 8u);
		meshletIndex -= 1; // remove valid check
		uint meshletPrimitiveIndex = (value >> 24u) & 0xFF;
		ShaderMeshlet meshlet = load_meshlet(meshletIndex);
		ShaderMeshInstance inst = load_instance(meshlet.instanceIndex);
		primitiveIndex = meshlet.primitiveOffset + meshletPrimitiveIndex;
		instanceIndex = meshlet.instanceIndex;
		subsetIndex = meshlet.geometryIndex - inst.geometryOffset;
	}

	// These packing methods don't need meshlets, but they are packed into 64 bits:
	uint2 pack2()
	{
		// 32 bit primitiveIndex + 1 valid check
		// 24 bit instanceIndex
		// 8  bit subsetIndex
		return uint2(primitiveIndex + 1, (instanceIndex & 0xFFFFFF) | ((subsetIndex & 0xFF) << 24u));
	}
	void unpack2(uint2 value)
	{
		primitiveIndex = value.x - 1; // remove valid check
		instanceIndex = value.y & 0xFFFFFF;
		subsetIndex = (value.y >> 24u) & 0xFF;
	}
};

#define texture_random64x64 bindless_textures[GetFrame().texture_random64x64_index]
#define texture_bluenoise bindless_textures[GetFrame().texture_bluenoise_index]
#define texture_sheenlut bindless_textures[GetFrame().texture_sheenlut_index]
#define texture_skyviewlut bindless_textures[GetFrame().texture_skyviewlut_index]
#define texture_transmittancelut bindless_textures[GetFrame().texture_transmittancelut_index]
#define texture_multiscatteringlut bindless_textures[GetFrame().texture_multiscatteringlut_index]
#define texture_skyluminancelut bindless_textures[GetFrame().texture_skyluminancelut_index]
#define texture_cameravolumelut bindless_textures3D[GetFrame().texture_cameravolumelut_index]
#define texture_wind bindless_textures3D[GetFrame().texture_wind_index]
#define texture_wind_prev bindless_textures3D[GetFrame().texture_wind_prev_index]
#define scene_acceleration_structure bindless_accelerationstructures[GetScene().TLAS]

#define texture_depth bindless_textures_float[GetCamera().texture_depth_index]
#define texture_depth_history bindless_textures_float[GetCamera().texture_depth_index_prev]
#define texture_lineardepth bindless_textures_float[GetCamera().texture_lineardepth_index]
#define texture_primitiveID bindless_textures_uint[GetCamera().texture_primitiveID_index]
#define texture_velocity bindless_textures_float2[GetCamera().texture_velocity_index]
#define texture_normal bindless_textures_float2[GetCamera().texture_normal_index]
#define texture_roughness bindless_textures_float[GetCamera().texture_roughness_index]

static const float PI = 3.14159265358979323846;
static const float SQRT2 = 1.41421356237309504880;
static const float FLT_MAX = 3.402823466e+38;
static const float FLT_EPSILON = 1.192092896e-07;
static const float GOLDEN_RATIO = 1.6180339887;
static const float M_TO_SKY_UNIT = 0.001f; // Engine units are in meters
static const float SKY_UNIT_TO_M = rcp(M_TO_SKY_UNIT);

#define sqr(a) ((a)*(a))
#define pow5(x) pow(x, 5)

// attribute computation with barycentric interpolation
//	a0 : attribute at triangle corner 0
//	a1 : attribute at triangle corner 1
//	a2 : attribute at triangle corner 2
//  bary : (u,v) barycentrics [same as you get from raytracing]; w is computed as 1 - u - w
//	computation can be also written as: p0 * w + p1 * u + p2 * v
inline float attribute_at_bary(in float a0, in float a1, in float a2, in float2 bary)
{
	return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}
inline float2 attribute_at_bary(in float2 a0, in float2 a1, in float2 a2, in float2 bary)
{
	return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}
inline float3 attribute_at_bary(in float3 a0, in float3 a1, in float3 a2, in float2 bary)
{
	return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}
inline float4 attribute_at_bary(in float4 a0, in float4 a1, in float4 a2, in float2 bary)
{
	return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}

inline bool is_saturated(float a) { return a == saturate(a); }
inline bool is_saturated(float2 a) { return all(a == saturate(a)); }
inline bool is_saturated(float3 a) { return all(a == saturate(a)); }
inline bool is_saturated(float4 a) { return all(a == saturate(a)); }

inline uint align(uint value, uint alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}
inline uint2 align(uint2 value, uint2 alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}
inline uint3 align(uint3 value, uint3 alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}
inline uint4 align(uint4 value, uint4 alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

inline float2 uv_to_clipspace(in float2 uv)
{
	float2 clipspace = uv * 2 - 1;
	clipspace.y *= -1;
	return clipspace;
}
inline float2 clipspace_to_uv(in float2 clipspace)
{
	return clipspace * float2(0.5, -0.5) + 0.5;
}
inline float3 clipspace_to_uv(in float3 clipspace)
{
	return clipspace * float3(0.5, -0.5, 0.5) + 0.5;
}

template<typename T>
T inverse_lerp(T value1, T value2, T pos)
{
	return all(value2 == value1) ? 0 : ((pos - value1) / (value2 - value1));
}

inline float3 GetSunColor() { return GetWeather().sun_color; } // sun color with intensity applied
inline float3 GetSunDirection() { return GetWeather().sun_direction; }
inline float3 GetHorizonColor() { return GetWeather().horizon.rgb; }
inline float3 GetZenithColor() { return GetWeather().zenith.rgb; }
inline float3 GetAmbientColor() { return GetWeather().ambient.rgb; }
inline uint2 GetInternalResolution() { return GetCamera().internal_resolution; }
inline float GetTime() { return GetFrame().time; }
inline float GetTimePrev() { return GetFrame().time_previous; }
inline uint2 GetTemporalAASampleRotation() { return uint2((GetFrame().temporalaa_samplerotation >> 0u) & 0x000000FF, (GetFrame().temporalaa_samplerotation >> 8) & 0x000000FF); }
inline bool IsStaticSky() { return GetScene().globalenvmap >= 0; }

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

// Random number generator based on: https://github.com/diharaw/helios/blob/master/src/engine/shader/random.glsl
struct RNG
{
	uint2 s; // state

	// xoroshiro64* random number generator.
	// http://prng.di.unimi.it/xoroshiro64star.c
	uint rotl(uint x, uint k)
	{
		return (x << k) | (x >> (32 - k));
	}
	// Xoroshiro64* RNG
	uint next()
	{
		uint result = s.x * 0x9e3779bb;

		s.y ^= s.x;
		s.x = rotl(s.x, 26) ^ s.y ^ (s.y << 9);
		s.y = rotl(s.y, 13);

		return result;
	}
	// Thomas Wang 32-bit hash.
	// http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
	uint hash(uint seed)
	{
		seed = (seed ^ 61) ^ (seed >> 16);
		seed *= 9;
		seed = seed ^ (seed >> 4);
		seed *= 0x27d4eb2d;
		seed = seed ^ (seed >> 15);
		return seed;
	}

	void init(uint2 id, uint frameIndex)
	{
		uint s0 = (id.x << 16) | id.y;
		uint s1 = frameIndex;
		s.x = hash(s0);
		s.y = hash(s1);
		next();
	}
	float next_float()
	{
		uint u = 0x3f800000 | (next() >> 9);
		return asfloat(u) - 1.0;
	}
	uint next_uint(uint nmax)
	{
		float f = next_float();
		return uint(floor(f * nmax));
	}
	float2 next_float2()
	{
		return float2(next_float(), next_float());
	}
	float3 next_float3()
	{
		return float3(next_float(), next_float(), next_float());
	}
};

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

// https://www.shadertoy.com/view/Msf3WH
float2 hash_simplex_2D(float2 p) // replace this by something better
{
	p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
	return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}
float noise_simplex_2D(in float2 p)
{
	const float K1 = 0.366025404; // (sqrt(3)-1)/2;
	const float K2 = 0.211324865; // (3-sqrt(3))/6;

	float2  i = floor(p + (p.x + p.y) * K1);
	float2  a = p - i + (i.x + i.y) * K2;
	float m = step(a.y, a.x);
	float2  o = float2(m, 1.0 - m);
	float2  b = a - o + K2;
	float2  c = a - 1.0 + 2.0 * K2;
	float3  h = max(0.5 - float3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	float3  n = h * h * h * h * float3(dot(a, hash_simplex_2D(i + 0.0)), dot(b, hash_simplex_2D(i + o)), dot(c, hash_simplex_2D(i + 1.0)));
	return dot(n, 70.0.xxx);
}

// https://www.shadertoy.com/view/Xsl3Dl
float3 hash_gradient_3D( float3 p ) // replace this by something better
{
	p = float3(dot(p, float3(127.1, 311.7, 74.7)),
		dot(p, float3(269.5, 183.3, 246.1)),
		dot(p, float3(113.5, 271.9, 124.6)));

	return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}
float noise_gradient_3D(in float3 p)
{
	float3 i = floor(p);
	float3 f = frac(p);

	float3 u = f * f * (3.0 - 2.0 * f);

	return lerp(lerp(lerp(dot(hash_gradient_3D(i + float3(0.0, 0.0, 0.0)), f - float3(0.0, 0.0, 0.0)),
		dot(hash_gradient_3D(i + float3(1.0, 0.0, 0.0)), f - float3(1.0, 0.0, 0.0)), u.x),
		lerp(dot(hash_gradient_3D(i + float3(0.0, 1.0, 0.0)), f - float3(0.0, 1.0, 0.0)),
			dot(hash_gradient_3D(i + float3(1.0, 1.0, 0.0)), f - float3(1.0, 1.0, 0.0)), u.x), u.y),
		lerp(lerp(dot(hash_gradient_3D(i + float3(0.0, 0.0, 1.0)), f - float3(0.0, 0.0, 1.0)),
			dot(hash_gradient_3D(i + float3(1.0, 0.0, 1.0)), f - float3(1.0, 0.0, 1.0)), u.x),
			lerp(dot(hash_gradient_3D(i + float3(0.0, 1.0, 1.0)), f - float3(0.0, 1.0, 1.0)),
				dot(hash_gradient_3D(i + float3(1.0, 1.0, 1.0)), f - float3(1.0, 1.0, 1.0)), u.x), u.y), u.z);
}


// Based on: https://www.shadertoy.com/view/MslGD8
float2 hash_voronoi(float2 p)
{
    //p = mod(p, 4.0); // tile
	p = float2(dot(p, float2(127.1, 311.7)),
             dot(p, float2(269.5, 183.3)));
	return frac(sin(p) * 18.5453);
}

// return distance, and cell id
float2 noise_voronoi(in float2 x, in float seed)
{
	float2 n = floor(x);
	float2 f = frac(x);

	float3 m = 8.0;
	for (int j = -1; j <= 1; j++)
		for (int i = -1; i <= 1; i++)
		{
			float2 g = float2(float(i), float(j));
			float2 o = hash_voronoi(n + g);
			//float2  r = g - f + o;
			float2 r = g - f + (0.5 + 0.5 * sin(seed + 6.2831 * o));
			float d = dot(r, r);
			if (d < m.x)
				m = float3(d, o);
		}

	return float2(sqrt(m.x), m.y + m.z);
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

// Computes post-projection depth from linear depth
inline float compute_inverse_lineardepth(in float lin, in float near, in float far)
{
	float z_n = ((lin - 2 * far) * near + far * lin) / (lin * near - far * lin);
	float z = (z_n + 1) / 2;
	return z;
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
inline float3 sample_hemisphere_uniform(in float3 normal, inout RNG rng)
{
	return mul(hemispherepoint_uniform(rng.next_float(), rng.next_float()), get_tangentspace(normal));
}
// Get random hemisphere sample in world-space along the normal (cosine-weighted distribution)
inline float3 sample_hemisphere_cos(in float3 normal, inout RNG rng)
{
	return mul(hemispherepoint_cos(rng.next_float(), rng.next_float()), get_tangentspace(normal));
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

inline float find_max_depth(in float2 uv, in int radius, in float lod)
{
	uint2 dim;
	texture_depth.GetDimensions(dim.x, dim.y);
	float2 dim_rcp = rcp(dim);
	float ret = 0;
	for (int x = -radius; x <= radius;++x)
	{
		for (int y = -radius; y <= radius;++y)
		{
			ret = max(ret, texture_depth.SampleLevel(sampler_point_clamp, uv + float2(x, y) * dim_rcp, lod));
		}
	}
	return ret;
}

// Caustic pattern from: https://www.shadertoy.com/view/XtKfRG
inline float caustic_pattern(float2 uv, float time)
{
	float3 k = float3(uv, time);
	float3x3 m = float3x3(-2, -1, 2, 3, -2, 1, 1, 2, 2);
	float3 a = mul(k, m) * 0.5;
	float3 b = mul(a, m) * 0.4;
	float3 c = mul(b, m) * 0.3;
	return pow(min(min(length(0.5 - frac(a)), length(0.5 - frac(b))), length(0.5 - frac(c))), 7) * 25.;
}

// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// uv			: UV texture coordinates on cubemap face in range [0, 1]
// faceIndex	: cubemap face index as in the backing texture2DArray in range [0, 5]
// returns direction to be used in cubemap sampling
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
// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// direction	: direction that is usable for cubemap sampling
// returns float3 that has uv in .xy components, and face index in Z component
//	https://stackoverflow.com/questions/53115467/how-to-implement-texturecube-using-6-sampler2d
inline float3 cubemap_to_uv(in float3 r)
{
	float faceIndex = 0;
	float3 absr = abs(r);
	float3 uvw = 0;
	if (absr.x > absr.y && absr.x > absr.z)
	{
		// x major
		float negx = step(r.x, 0.0);
		uvw = float3(r.zy, absr.x) * float3(lerp(-1.0, 1.0, negx), -1, 1);
		faceIndex = negx;
	}
	else if (absr.y > absr.z)
	{
		// y major
		float negy = step(r.y, 0.0);
		uvw = float3(r.xz, absr.y) * float3(1.0, lerp(1.0, -1.0, negy), 1.0);
		faceIndex = 2.0 + negy;
	}
	else
	{
		// z major
		float negz = step(r.z, 0.0);
		uvw = float3(r.xy, absr.z) * float3(lerp(1.0, -1.0, negz), -1, 1);
		faceIndex = 4.0 + negz;
	}
	return float3((uvw.xy / uvw.z + 1) * 0.5, faceIndex);
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
	retVal |= (uint)((value.x) * 255.0) << 0u;
	retVal |= (uint)((value.y) * 255.0) << 8u;
	retVal |= (uint)((value.z) * 255.0) << 16u;
	retVal |= (uint)((value.w) * 255.0) << 24u;
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

inline uint pack_pixel(uint2 value)
{
	return (value.x & 0xFFFF) | ((value.y & 0xFFFF) << 16u);
}
inline uint2 unpack_pixel(uint value)
{
	uint2 retVal;
	retVal.x = value & 0xFFFF;
	retVal.y = (value >> 16u) & 0xFFFF;
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

// Source: https://github.com/GPUOpen-Effects/FidelityFX-Denoiser/blob/master/ffx-shadows-dnsr/ffx_denoiser_shadows_util.h
//  LANE TO 8x8 MAPPING
//  ===================
//  00 01 08 09 10 11 18 19 
//  02 03 0a 0b 12 13 1a 1b
//  04 05 0c 0d 14 15 1c 1d
//  06 07 0e 0f 16 17 1e 1f 
//  20 21 28 29 30 31 38 39 
//  22 23 2a 2b 32 33 3a 3b
//  24 25 2c 2d 34 35 3c 3d
//  26 27 2e 2f 36 37 3e 3f 
uint bitfield_extract(uint src, uint off, uint bits) { uint mask = (1u << bits) - 1; return (src >> off) & mask; } // ABfe
uint bitfield_insert(uint src, uint ins, uint bits) { uint mask = (1u << bits) - 1; return (ins & mask) | (src & (~mask)); } // ABfiM
uint2 remap_lane_8x8(uint lane) {
	return uint2(bitfield_insert(bitfield_extract(lane, 2u, 3u), lane, 1u)
		, bitfield_insert(bitfield_extract(lane, 3u, 3u)
			, bitfield_extract(lane, 1u, 2u), 2u));
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


inline float sphere_surface_area(in float radius)
{
	return 4 * PI * radius * radius;
}
inline float sphere_volume(in float radius)
{
	return 4.0 / 3.0 * PI * radius * radius * radius;
}


float plane_point_distance(float3 planeOrigin, float3 planeNormal, float3 P)
{
	return dot(planeNormal, P - planeOrigin);
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
// Compute barycentric coordinates on triangle from a ray
float2 compute_barycentrics(float3 rayOrigin, float3 rayDirection, float3 a, float3 b, float3 c)
{
	float3 v0v1 = b - a;
	float3 v0v2 = c - a;
	float3 pvec = cross(rayDirection, v0v2);
	float det = dot(v0v1, pvec);
	float det_rcp = rcp(det);
	float3 tvec = rayOrigin - a;
	float u = dot(tvec, pvec) * det_rcp;
	float3 qvec = cross(tvec, v0v1);
	float v = dot(rayDirection, qvec) * det_rcp;
	return float2(u, v);
}
// Compute barycentric coordinates on triangle from a ray
//	also outputs hit distance "t"
float2 compute_barycentrics(float3 rayOrigin, float3 rayDirection, float3 a, float3 b, float3 c, out float t)
{
	float3 v0v1 = b - a;
	float3 v0v2 = c - a;
	float3 pvec = cross(rayDirection, v0v2);
	float det = dot(v0v1, pvec);
	float det_rcp = rcp(det);
	float3 tvec = rayOrigin - a;
	float u = dot(tvec, pvec) * det_rcp;
	float3 qvec = cross(tvec, v0v1);
	float v = dot(rayDirection, qvec) * det_rcp;
	t = dot(v0v2, qvec) * det_rcp;
	return float2(u, v);
}
// Compute barycentric coordinates on triangle from a ray
//	also outputs hit distance "t"
//	also outputs backface flag
float2 compute_barycentrics(float3 rayOrigin, float3 rayDirection, float3 a, float3 b, float3 c, out float t, out bool is_backface)
{
	float3 v0v1 = b - a;
	float3 v0v2 = c - a;
	float3 pvec = cross(rayDirection, v0v2);
	float det = dot(v0v1, pvec);
	is_backface = det > 0;
	float det_rcp = rcp(det);
	float3 tvec = rayOrigin - a;
	float u = dot(tvec, pvec) * det_rcp;
	float3 qvec = cross(tvec, v0v1);
	float v = dot(rayDirection, qvec) * det_rcp;
	t = dot(v0v2, qvec) * det_rcp;
	return float2(u, v);
}

// Texture LOD computation things from https://github.com/EmbarkStudios/kajiya
float twice_triangle_area(float3 p0, float3 p1, float3 p2)
{
	return length(cross(p1 - p0, p2 - p0));
}
float twice_uv_area(float2 t0, float2 t1, float2 t2)
{
	return abs((t1.x - t0.x) * (t2.y - t0.y) - (t2.x - t0.x) * (t1.y - t0.y));
}
// https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
float compute_texture_lod(Texture2D tex, float triangle_constant, float3 ray_direction, float3 surf_normal, float cone_width)
{
	uint w, h;
	tex.GetDimensions(w, h);

	float lambda = triangle_constant;
	lambda += log2(abs(cone_width));
	lambda += 0.5 * log2(float(w) * float(h));
	lambda -= log2(abs(dot(normalize(ray_direction), surf_normal)));
	return lambda;
}
float pixel_cone_spread_angle_from_image_height(float image_height)
{
	//return atan(2.0 * frame_constants.view_constants.clip_to_view._11 / image_height);
	return atan(2.0 * GetCamera().inverse_projection._11 / image_height);
}
// https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
struct RayCone
{
	float width;
	float spread_angle;

	static RayCone from_spread_angle(float spread_angle)
	{
		RayCone res;
		res.width = 0.0;
		res.spread_angle = spread_angle;
		return res;
	}

	static RayCone from_width_spread_angle(float width, float spread_angle)
	{
		RayCone res;
		res.width = width;
		res.spread_angle = spread_angle;
		return res;
	}

	RayCone propagate(float surface_spread_angle, float hit_t)
	{
		RayCone res;
		res.width = this.spread_angle * hit_t + this.width;
		res.spread_angle = this.spread_angle + surface_spread_angle;
		return res;
	}

	float width_at_t(float hit_t)
	{
		return this.width + this.spread_angle * hit_t;
	}
};
RayCone pixel_ray_cone_from_image_height(float image_height)
{
	RayCone res;
	res.width = 0.0;
	res.spread_angle = pixel_cone_spread_angle_from_image_height(image_height);
	return res;
}


float3 sample_wind(float3 position, float weight)
{
	[branch]
	if (weight > 0)
	{
		return texture_wind.SampleLevel(sampler_linear_mirror, position, 0).r * GetWeather().wind.direction * weight;
	}
	else
	{
		return 0;
	}
}
float3 sample_wind_prev(float3 position, float weight)
{
	[branch]
	if (weight > 0)
	{
		return texture_wind_prev.SampleLevel(sampler_linear_mirror, position, 0).r * GetWeather().wind.direction * weight;
	}
	else
	{
		return 0;
	}
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


static const uint NUM_PARALLAX_OCCLUSION_STEPS = 32;
inline void ParallaxOcclusionMapping_Impl(
	inout float4 uvsets,		// uvsets to modify
	in float3 V,				// view vector (pointing towards camera)
	in float3x3 TBN,			// tangent basis matrix (same that is used for normal mapping)
	in ShaderMaterial material,	// material parameters
	in Texture2D tex,			// displacement map texture
	in float2 uv,				// uv to use for the displacement map
	in float2 uv_dx,			// horizontal derivative of displacement map uv
	in float2 uv_dy				// vertical derivative of displacement map uv
)
{
	[branch]
	if (material.parallaxOcclusionMapping > 0)
	{
		V = mul(TBN, V);
		float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
		float curLayerHeight = 0;
		float2 dtex = material.parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
		float2 currentTextureCoords = uv;
		float heightFromTexture = 1 - tex.SampleGrad(sampler_linear_wrap, currentTextureCoords, uv_dx, uv_dy).r;
		uint iter = 0;
		[loop]
		while (heightFromTexture > curLayerHeight && iter < NUM_PARALLAX_OCCLUSION_STEPS)
		{
			curLayerHeight += layerHeight;
			currentTextureCoords -= dtex;
			heightFromTexture = 1 - tex.SampleGrad(sampler_linear_wrap, currentTextureCoords, uv_dx, uv_dy).r;
			iter++;
		}
		float2 prevTCoords = currentTextureCoords + dtex;
		float nextH = heightFromTexture - curLayerHeight;
		float prevH = 1 - tex.SampleGrad(sampler_linear_wrap, prevTCoords, uv_dx, uv_dy).r - curLayerHeight + layerHeight;
		float weight = nextH / (nextH - prevH);
		float2 finalTextureCoords = mad(prevTCoords, weight, currentTextureCoords * (1.0 - weight));
		float2 difference = finalTextureCoords - uv;
		uvsets += difference.xyxy;
	}
}

#endif // WI_SHADER_GLOBALS_HF
