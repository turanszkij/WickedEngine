#ifndef WI_SHADERINTEROP_H
#define WI_SHADERINTEROP_H

#include "ShaderInterop_Vulkan.h"
#include "ConstantBufferMapping.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"


#ifdef __cplusplus // not invoking shader compiler, but included in engine source

// Application-side types:

typedef XMMATRIX matrix;
typedef XMFLOAT4X4 float4x4;
typedef XMFLOAT2 float2;
typedef XMFLOAT3 float3;
typedef XMFLOAT4 float4;
typedef uint32_t uint;
typedef XMUINT2 uint2;
typedef XMUINT3 uint3;
typedef XMUINT4 uint4;
typedef XMINT2 int2;
typedef XMINT3 int3;
typedef XMINT4 int4;

#define CB_GETBINDSLOT(name) __CBUFFERBINDSLOT__##name##__
#define CBUFFER(name, slot) static const int CB_GETBINDSLOT(name) = slot; struct alignas(16) name

#else

// Shader - side types:

#ifdef SPIRV // invoking Vulkan shader compiler (HLSL -> SPIRV)

#define globallycoherent /*nothing*/

#define CBUFFER(name, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_B + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] cbuffer name

#define RAWBUFFER(name,slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] ByteAddressBuffer name
#define RWRAWBUFFER(name,slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWByteAddressBuffer name

#define TYPEDBUFFER(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Buffer< type > name
#define RWTYPEDBUFFER(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWBuffer< type > name

#define STRUCTUREDBUFFER(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] StructuredBuffer< type > name
#define RWSTRUCTUREDBUFFER(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWStructuredBuffer< type > name
#define ROVSTRUCTUREDBUFFER(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RasterizerOrderedStructuredBuffer< type > name

#define RAYTRACINGACCELERATIONSTRUCTURE(name, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RaytracingAccelerationStructure name


#define TEXTURE1D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture1D< type > name
#define TEXTURE1DARRAY(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture1DArray< type > name
#define RWTEXTURE1D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWTexture1D< type > name

#define TEXTURE2D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture2D< type > name
#define TEXTURE2DMS(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture2DMS< type > name
#define TEXTURE2DARRAY(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture2DArray< type > name
#define RWTEXTURE2D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWTexture2D< type > name
#define RWTEXTURE2DARRAY(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWTexture2DArray< type > name
#define ROVTEXTURE2D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RasterizerOrderedTexture2D< type > name

#define TEXTURECUBE(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] TextureCube< type > name
#define TEXTURECUBEARRAY(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] TextureCubeArray< type > name

#define TEXTURE3D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_T + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] Texture3D< type > name
#define RWTEXTURE3D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RWTexture3D< type > name
#define ROVTEXTURE3D(name, type, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_U + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] RasterizerOrderedTexture3D< type > name


#define SAMPLERSTATE(name, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_S + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] SamplerState name
#define SAMPLERCOMPARISONSTATE(name, slot) [[vk::binding(slot + VULKAN_BINDING_SHIFT_S + VULKAN_BINDING_SHIFT_STAGE(SPIRV_SHADERSTAGE))]] SamplerComparisonState name



#else // invoking DirectX shader compiler

#define CBUFFER(name, slot) cbuffer name : register(b ## slot)

#define RAWBUFFER(name,slot) ByteAddressBuffer name : register(t ## slot)
#define RWRAWBUFFER(name,slot) RWByteAddressBuffer name : register(u ## slot)

#define TYPEDBUFFER(name, type, slot) Buffer< type > name : register(t ## slot)
#define RWTYPEDBUFFER(name, type, slot) RWBuffer< type > name : register(u ## slot)

#define STRUCTUREDBUFFER(name, type, slot) StructuredBuffer< type > name : register(t ## slot)
#define RWSTRUCTUREDBUFFER(name, type, slot) RWStructuredBuffer< type > name : register(u ## slot)
#define ROVSTRUCTUREDBUFFER(name, type, slot) RasterizerOrderedStructuredBuffer< type > name : register(u ## slot)


#define TEXTURE1D(name, type, slot) Texture1D< type > name : register(t ## slot)
#define TEXTURE1DARRAY(name, type, slot) Texture1DArray< type > name : register(t ## slot)
#define RWTEXTURE1D(name, type, slot) RWTexture1D< type > name : register(u ## slot)

#define TEXTURE2D(name, type, slot) Texture2D< type > name : register(t ## slot)
#define TEXTURE2DMS(name, type, slot) Texture2DMS< type > name : register(t ## slot)
#define TEXTURE2DARRAY(name, type, slot) Texture2DArray< type > name : register(t ## slot)
#define RWTEXTURE2D(name, type, slot) RWTexture2D< type > name : register(u ## slot)
#define RWTEXTURE2DARRAY(name, type, slot) RWTexture2DArray< type > name : register(u ## slot)
#define ROVTEXTURE2D(name, type, slot) RasterizerOrderedTexture2D< type > name : register(u ## slot)

#define TEXTURECUBE(name, type, slot) TextureCube< type > name : register(t ## slot)
#define TEXTURECUBEARRAY(name, type, slot) TextureCubeArray< type > name : register(t ## slot)

#define TEXTURE3D(name, type, slot) Texture3D< type > name : register(t ## slot)
#define RWTEXTURE3D(name, type, slot) RWTexture3D< type > name : register(u ## slot)
#define ROVTEXTURE3D(name, type, slot) RasterizerOrderedTexture3D< type > name : register(u ## slot)


#define SAMPLERSTATE(name, slot) SamplerState name : register(s ## slot)
#define SAMPLERCOMPARISONSTATE(name, slot) SamplerComparisonState name : register(s ## slot)

#ifdef HLSL6
#define RAYTRACINGACCELERATIONSTRUCTURE(name, slot) RaytracingAccelerationStructure name : register(t ## slot)

#else
#define RAYTRACINGACCELERATIONSTRUCTURE(name, slot) 
#define WaveReadLaneFirst(a) (a)
#define WaveActiveBitOr(a) (a)
#endif // HLSL6

#endif // invoking vulkan/directx

#endif // __cplusplus

#endif // WI_SHADERINTEROP_H
