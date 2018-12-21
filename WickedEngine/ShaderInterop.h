#ifndef _WICKEDENGINE_SHADERINTEROP_H_
#define _WICKEDENGINE_SHADERINTEROP_H_

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

#ifdef SHADERCOMPILER_SPIRV // invoking Vulkan shader compiler (HLSL -> SPIRV)

#if defined(SPIRV_SHADERTYPE_VS)
#define VULKAN_DESCRIPTOR_SET_ID 0
#elif defined(SPIRV_SHADERTYPE_HS)
#define VULKAN_DESCRIPTOR_SET_ID 1
#elif defined(SPIRV_SHADERTYPE_DS)
#define VULKAN_DESCRIPTOR_SET_ID 2
#elif defined(SPIRV_SHADERTYPE_GS)
#define VULKAN_DESCRIPTOR_SET_ID 3
#elif defined(SPIRV_SHADERTYPE_PS)
#define VULKAN_DESCRIPTOR_SET_ID 4
#elif defined(SPIRV_SHADERTYPE_CS) // cs is different in that it only has single descriptor table, so we will index the first
#define VULKAN_DESCRIPTOR_SET_ID 0
#else
#error You must specify a shader type when compiling spirv to resolve descriptor sets! (eg. #define SPIRV_SHADERTYPE_VS)
#endif

#define globallycoherent /*nothing*/

#define CBUFFER(name, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_CBV + slot, VULKAN_DESCRIPTOR_SET_ID)]] cbuffer name

#define RAWBUFFER(name,slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] ByteAddressBuffer name
#define RWRAWBUFFER(name,slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWByteAddressBuffer name

#define TYPEDBUFFER(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] Buffer< type > name
#define RWTYPEDBUFFER(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWBuffer< type > name

#define STRUCTUREDBUFFER(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_UNTYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] StructuredBuffer< type > name
#define RWSTRUCTUREDBUFFER(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWStructuredBuffer< type > name
#define ROVSTRUCTUREDBUFFER(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_UNTYPEDBUFFER + slot, VULKAN_DESCRIPTOR_SET_ID)]] RasterizerOrderedStructuredBuffer< type > name


#define TEXTURE1D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture1D< type > name;
#define TEXTURE1DARRAY(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture1DArray< type > name;
#define RWTEXTURE1D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWTexture1D< type > name;

#define TEXTURE2D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture2D< type > name;
#define TEXTURE2DMS(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture2DMS< type > name;
#define TEXTURE2DARRAY(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture2DArray< type > name;
#define RWTEXTURE2D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWTexture2D< type > name;
#define RWTEXTURE2DARRAY(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWTexture2DArray< type > name;
#define ROVTEXTURE2D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RasterizerOrderedTexture2D< type > name;

#define TEXTURECUBE(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] TextureCube< type > name;
#define TEXTURECUBEARRAY(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] TextureCubeArray< type > name;

#define TEXTURE3D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SRV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] Texture3D< type > name;
#define RWTEXTURE3D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RWTexture3D< type > name;
#define ROVTEXTURE3D(name, type, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_UAV_TEXTURE + slot, VULKAN_DESCRIPTOR_SET_ID)]] RasterizerOrderedTexture3D< type > name;


#define SAMPLERSTATE(name, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER + slot, VULKAN_DESCRIPTOR_SET_ID)]] SamplerState name;
#define SAMPLERCOMPARISONSTATE(name, slot) [[vk::binding(VULKAN_DESCRIPTOR_SET_OFFSET_SAMPLER + slot, VULKAN_DESCRIPTOR_SET_ID)]] SamplerComparisonState name;



#else // invoking DirectX shader compiler


#define CBUFFER(name, slot) cbuffer name : register(b ## slot)

#define RAWBUFFER(name,slot) ByteAddressBuffer name : register(t ## slot)
#define RWRAWBUFFER(name,slot) RWByteAddressBuffer name : register(u ## slot)

#define TYPEDBUFFER(name, type, slot) Buffer< type > name : register(t ## slot)
#define RWTYPEDBUFFER(name, type, slot) RWBuffer< type > name : register(u ## slot)

#define STRUCTUREDBUFFER(name, type, slot) StructuredBuffer< type > name : register(t ## slot)
#define RWSTRUCTUREDBUFFER(name, type, slot) RWStructuredBuffer< type > name : register(u ## slot)
#define ROVSTRUCTUREDBUFFER(name, type, slot) RasterizerOrderedStructuredBuffer< type > name : register(u ## slot)


#define TEXTURE1D(name, type, slot) Texture1D< type > name : register(t ## slot);
#define TEXTURE1DARRAY(name, type, slot) Texture1DArray< type > name : register(t ## slot);
#define RWTEXTURE1D(name, type, slot) RWTexture1D< type > name : register(u ## slot);

#define TEXTURE2D(name, type, slot) Texture2D< type > name : register(t ## slot);
#define TEXTURE2DMS(name, type, slot) Texture2DMS< type > name : register(t ## slot);
#define TEXTURE2DARRAY(name, type, slot) Texture2DArray< type > name : register(t ## slot);
#define RWTEXTURE2D(name, type, slot) RWTexture2D< type > name : register(u ## slot);
#define RWTEXTURE2DARRAY(name, type, slot) RWTexture2DArray< type > name : register(u ## slot);
#define ROVTEXTURE2D(name, type, slot) RasterizerOrderedTexture2D< type > name : register(u ## slot);

#define TEXTURECUBE(name, type, slot) TextureCube< type > name : register(t ## slot);
#define TEXTURECUBEARRAY(name, type, slot) TextureCubeArray< type > name : register(t ## slot);

#define TEXTURE3D(name, type, slot) Texture3D< type > name : register(t ## slot);
#define RWTEXTURE3D(name, type, slot) RWTexture3D< type > name : register(u ## slot);
#define ROVTEXTURE3D(name, type, slot) RasterizerOrderedTexture3D< type > name : register(u ## slot);


#define SAMPLERSTATE(name, slot) SamplerState name : register(s ## slot);
#define SAMPLERCOMPARISONSTATE(name, slot) SamplerComparisonState name : register(s ## slot);

#endif // invoking vulkan/directx

#endif // __cplusplus


#define ENTITY_TYPE_DIRECTIONALLIGHT	0
#define ENTITY_TYPE_POINTLIGHT			1
#define ENTITY_TYPE_SPOTLIGHT			2
#define ENTITY_TYPE_SPHERELIGHT			3
#define ENTITY_TYPE_DISCLIGHT			4
#define ENTITY_TYPE_RECTANGLELIGHT		5
#define ENTITY_TYPE_TUBELIGHT			6
#define ENTITY_TYPE_DECAL				100
#define ENTITY_TYPE_ENVMAP				101
#define ENTITY_TYPE_FORCEFIELD_POINT	200
#define ENTITY_TYPE_FORCEFIELD_PLANE	201

#define ENTITY_FLAG_LIGHT_STATIC		1 << 0

struct ShaderEntityType
{
	uint params;
	float3 positionVS;
	float range;
	float3 directionVS;
	float3 positionWS;
	float energy;
	uint color;
	float3 directionWS;
	float coneAngleCos;
	float shadowKernel;
	float shadowBias;
	int additionalData_index;
	float4 texMulAdd;

	inline uint GetType()
	{
		return params & 0xFFFF;
	}
	inline uint GetFlags()
	{
		return (params >> 16) & 0xFFFF;
	}

	inline void SetType(uint type)
	{
		params = type & 0xFFFF; // also initializes to zero, so flags must be set after the type
	}
	inline void SetFlags(uint flags)
	{
		params |= (flags & 0xFFFF) << 16;
	}

	// Load uncompressed color:
	inline float4 GetColor()
	{
		float4 fColor;

		fColor.x = (float)((color >> 0) & 0x000000FF) / 255.0f;
		fColor.y = (float)((color >> 8) & 0x000000FF) / 255.0f;
		fColor.z = (float)((color >> 16) & 0x000000FF) / 255.0f;
		fColor.w = (float)((color >> 24) & 0x000000FF) / 255.0f;

		return fColor;
	}

	// Load area light props:
	inline float3 GetRight() { return directionWS; }
	inline float3 GetUp() { return directionVS; }
	inline float3 GetFront() { return positionVS; }
	inline float GetRadius() { return texMulAdd.x; }
	inline float GetWidth() { return texMulAdd.y; }
	inline float GetHeight() { return texMulAdd.z; }

	// Load decal props:
	inline float GetEmissive() { return energy; }
};


// Tiled rendering params:
#define TILED_CULLING_BLOCKSIZE	16
#define MAX_SHADER_ENTITY_COUNT	4096
#define MAX_SHADER_ENTITY_COUNT_PER_TILE 256

#define MATRIXARRAY_COUNT	128


#endif
