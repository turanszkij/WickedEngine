#ifndef WI_SHADERINTEROP_H
#define WI_SHADERINTEROP_H

#ifdef __cplusplus // not invoking shader compiler, but included in engine source

// Application-side types:
#include "../wiMath.h"

using float4x4 = XMFLOAT4X4;
using float2 = XMFLOAT2;
using float3 = XMFLOAT3;
using float4 = XMFLOAT4;
using uint = uint32_t;
using uint2 = XMUINT2;
using uint3 = XMUINT3;
using uint4 = XMUINT4;
using int2 = XMINT2;
using int3 = XMINT3;
using int4 = XMINT4;

#define column_major
#define row_major

#define CB_GETBINDSLOT(name) __CBUFFERBINDSLOT__##name##__
#define CBUFFER(name, slot) static const int CB_GETBINDSLOT(name) = slot; struct alignas(16) name
#define CONSTANTBUFFER(name, type, slot)
#define PUSHCONSTANT(name, type)

#else

// Shader - side types:

#define PASTE1(a, b) a##b
#define PASTE(a, b) PASTE1(a, b)
#define CBUFFER(name, slot) cbuffer name : register(PASTE(b, slot))
#define CONSTANTBUFFER(name, type, slot) ConstantBuffer< type > name : register(PASTE(b, slot))

#ifndef SPIRV
#ifdef __spirv__
#define SPIRV
#endif // __spirv__
#endif // SPIRV

#ifdef SPIRV
#define PUSHCONSTANT(name, type) [[vk::push_constant]] type name;
#else
#define PUSHCONSTANT(name, type) ConstantBuffer<type> name : register(b999)
#endif // HLSL6

#endif // __cplusplus

struct IndirectDrawArgsInstanced
{
	uint VertexCountPerInstance;
	uint InstanceCount;
	uint StartVertexLocation;
	uint StartInstanceLocation;
};
struct IndirectDrawArgsIndexedInstanced
{
	uint IndexCountPerInstance;
	uint InstanceCount;
	uint StartIndexLocation;
	int BaseVertexLocation;
	uint StartInstanceLocation;
};
struct IndirectDispatchArgs
{
	uint ThreadGroupCountX;
	uint ThreadGroupCountY;
	uint ThreadGroupCountZ;
};


// Common buffers:
// These are usable by all shaders
#define CBSLOT_IMAGE							0
#define CBSLOT_FONT								0
#define CBSLOT_RENDERER_FRAME					0
#define CBSLOT_RENDERER_CAMERA					1

// On demand buffers:
// These are bound on demand and alive until another is bound at the same slot

#define CBSLOT_RENDERER_FORWARD_LIGHTMASK		2
#define CBSLOT_RENDERER_VOLUMELIGHT				3
#define CBSLOT_RENDERER_VOXELIZER				3
#define CBSLOT_RENDERER_TRACED					2
#define CBSLOT_RENDERER_MISC					3

#define CBSLOT_OTHER_EMITTEDPARTICLE			4
#define CBSLOT_OTHER_HAIRPARTICLE				4
#define CBSLOT_OTHER_FFTGENERATOR				3
#define CBSLOT_OTHER_OCEAN_SIMULATION_IMMUTABLE	3
#define CBSLOT_OTHER_OCEAN_SIMULATION_PERFRAME	4
#define CBSLOT_OTHER_OCEAN_RENDER				3
#define CBSLOT_OTHER_CLOUDGENERATOR				3
#define CBSLOT_OTHER_GPUSORTLIB					4
#define CBSLOT_MSAO								4
#define CBSLOT_FSR								4


#endif // WI_SHADERINTEROP_H
