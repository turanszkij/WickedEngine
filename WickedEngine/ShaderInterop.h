#ifndef _WICKEDENGINE_SHADERINTEROP_H_
#define _WICKEDENGINE_SHADERINTEROP_H_

#include "ConstantBufferMapping.h"
#include "SamplerMapping.h"
#include "ResourceMapping.h"


#ifdef __cplusplus

// Application-side types:

#define matrix XMMATRIX
#define float4x4 XMFLOAT4X4
#define float2 XMFLOAT2
#define float3 XMFLOAT3
#define float4 XMFLOAT4
#define uint uint32_t
#define uint2 XMUINT2
#define uint3 XMUINT3
#define uint4 XMUINT4

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



struct LightArrayType
{
	float3 positionVS; // View Space!
	float range;
	// --
	float4 color;
	// --
	float3 positionWS;
	float energy;
	// --
	float3 directionVS;
	float shadowKernel;
	// --
	float3 directionWS;
	uint type;
	// --
	float shadowBias;
	int shadowMap_index;
	float coneAngle;
	float coneAngleCos;
	// --
	float4 texMulAdd;
	// --
	matrix shadowMatrix[3];
};



// Tiled rendering params:
#define TILED_CULLING_BLOCKSIZE	16
#define MAX_LIGHTS	1024


// MIP Generator params:
#define GENERATEMIPCHAIN_1D_BLOCK_SIZE 32
#define GENERATEMIPCHAIN_2D_BLOCK_SIZE 16
#define GENERATEMIPCHAIN_3D_BLOCK_SIZE 8

// Skinning compute params:
#define SKINNING_COMPUTE_THREADCOUNT 1024

// Grass culling params:
#define GRASS_CULLING_THREADCOUNT 256

// Bitonic sort params:
#define BITONIC_BLOCK_SIZE 512
#define TRANSPOSE_BLOCK_SIZE 16

#endif
