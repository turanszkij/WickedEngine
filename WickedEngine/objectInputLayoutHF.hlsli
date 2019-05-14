#ifndef _MESH_INPUT_LAYOUT_HF_
#define _MESH_INPUT_LAYOUT_HF_
#include "ShaderInterop_Renderer.h"

struct Input_Instance
{
	float4 mat0 : INSTANCEMATRIX0;
	float4 mat1 : INSTANCEMATRIX1;
	float4 mat2 : INSTANCEMATRIX2;
	float4 color : INSTANCECOLOR;
};
struct Input_InstancePrev
{
	float4 matPrev0 : INSTANCEMATRIXPREV0;
	float4 matPrev1 : INSTANCEMATRIXPREV1;
	float4 matPrev2 : INSTANCEMATRIXPREV2;
};
struct Input_InstanceAtlas
{
	float4 atlasMulAdd : INSTANCEATLAS;
};

struct Input_Object_POS
{
	float4 pos : POSITION_NORMAL_SUBSETINDEX;
	Input_Instance inst;
};
struct Input_Object_POS_TEX
{
	float4 pos : POSITION_NORMAL_SUBSETINDEX;
	float2 uv0 : UVSET0;
	float2 uv1 : UVSET1;
	Input_Instance inst;
};
struct Input_Object_ALL
{
	float4 pos : POSITION_NORMAL_SUBSETINDEX;
	float2 uv0 : UVSET0;
	float2 uv1 : UVSET1;
	float2 atl : ATLAS;
	float4 col : COLOR;
	float4 pre : PREVPOS;
	Input_Instance inst;
	Input_InstancePrev instPrev;
	Input_InstanceAtlas instAtlas;
};

inline float4x4 MakeWorldMatrixFromInstance(in Input_Instance input)
{
	return float4x4(
		  float4(input.mat0.x, input.mat1.x, input.mat2.x, 0)
		, float4(input.mat0.y, input.mat1.y, input.mat2.y, 0)
		, float4(input.mat0.z, input.mat1.z, input.mat2.z, 0)
		, float4(input.mat0.w, input.mat1.w, input.mat2.w, 1)
		);
}
inline float4x4 MakeWorldMatrixFromInstance(in Input_InstancePrev input)
{
	return float4x4(
		  float4(input.matPrev0.x, input.matPrev1.x, input.matPrev2.x, 0)
		, float4(input.matPrev0.y, input.matPrev1.y, input.matPrev2.y, 0)
		, float4(input.matPrev0.z, input.matPrev1.z, input.matPrev2.z, 0)
		, float4(input.matPrev0.w, input.matPrev1.w, input.matPrev2.w, 1)
		);
}

struct VertexSurface
{
	float4 position;
	float4 uvsets;
	float2 atlas;
	float4 color;
	float3 normal;
	uint subsetIndex;
	float4 prevPos;
};
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_POS input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	surface.color = g_xMat_baseColor * input.inst.color;

	uint normal_subsetIndex = asuint(input.pos.w);
	surface.normal.x = (float)((normal_subsetIndex >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_subsetIndex >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_subsetIndex >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.subsetIndex = (normal_subsetIndex >> 24) & 0x000000FF;

	return surface;
}
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_POS_TEX input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	surface.color = g_xMat_baseColor * input.inst.color;

	uint normal_subsetIndex = asuint(input.pos.w);
	surface.normal.x = (float)((normal_subsetIndex >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_subsetIndex >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_subsetIndex >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.subsetIndex = (normal_subsetIndex >> 24) & 0x000000FF;

	surface.uvsets = float4(input.uv0 * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw, input.uv1);

	return surface;
}
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_ALL input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	surface.color = g_xMat_baseColor * input.inst.color;

	if (g_xMat_useVertexColors)
	{
		surface.color *= input.col;
	}

	uint normal_subsetIndex = asuint(input.pos.w);
	surface.normal.x = (float)((normal_subsetIndex >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_subsetIndex >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_subsetIndex >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.subsetIndex = (normal_subsetIndex >> 24) & 0x000000FF;

	surface.uvsets = float4(input.uv0 * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw, input.uv1);

	surface.atlas = input.atl * input.instAtlas.atlasMulAdd.xy + input.instAtlas.atlasMulAdd.zw;

	surface.prevPos = float4(input.pre.xyz, 1);

	return surface;
}

#endif // _MESH_INPUT_LAYOUT_HF_