#ifndef _WICKEDENGINE_SHADERINTEROP_H_
#define _WICKEDENGINE_SHADERINTEROP_H_

#include "ConstantBufferMapping.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"


#ifdef __cplusplus

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
#define ROVTEXTURE2D(name, type, slot) RasterizerOrderedTexture2D< type > name : register(u ## slot);

#define TEXTURECUBE(name, type, slot) TextureCube< type > name : register(t ## slot);
#define TEXTURECUBEARRAY(name, type, slot) TextureCubeArray< type > name : register(t ## slot);

#define TEXTURE3D(name, type, slot) Texture3D< type > name : register(t ## slot);
#define RWTEXTURE3D(name, type, slot) RWTexture3D< type > name : register(u ## slot);
#define ROVTEXTURE3D(name, type, slot) RasterizerOrderedTexture3D< type > name : register(u ## slot);


#define SAMPLERSTATE(name, slot) SamplerState name : register(s ## slot);
#define SAMPLERCOMPARISONSTATE(name, slot) SamplerComparisonState name : register(s ## slot);

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

struct ShaderEntityType
{
	uint type;
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


// MIP Generator params:
#define GENERATEMIPCHAIN_1D_BLOCK_SIZE 32
#define GENERATEMIPCHAIN_2D_BLOCK_SIZE 16
#define GENERATEMIPCHAIN_3D_BLOCK_SIZE 8

// Skinning compute params:
#define SKINNING_COMPUTE_THREADCOUNT 256


#endif
