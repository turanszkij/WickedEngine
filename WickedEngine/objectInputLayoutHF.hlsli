#ifndef WI_MESH_INPUT_LAYOUT_HF
#define WI_MESH_INPUT_LAYOUT_HF
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

struct Input_Instance
{
	float4 mat0 : INSTANCEMATRIX0;
	float4 mat1 : INSTANCEMATRIX1;
	float4 mat2 : INSTANCEMATRIX2;
	uint4 userdata : INSTANCEUSERDATA;
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
	float4 pos : POSITION_NORMAL_WIND;
	Input_Instance inst;
};
struct Input_Object_POS_TEX
{
	float4 pos : POSITION_NORMAL_WIND;
	float2 uv0 : UVSET0;
	float2 uv1 : UVSET1;
	Input_Instance inst;
};
struct Input_Object_ALL
{
	float4 pos : POSITION_NORMAL_WIND;
	float2 uv0 : UVSET0;
	float2 uv1 : UVSET1;
	float2 atl : ATLAS;
	float4 col : COLOR;
	float4 tan : TANGENT;
	float4 pre : PREVPOS;
	Input_Instance inst;
	Input_InstancePrev instPrev;
	Input_InstanceAtlas instAtlas;
};

inline float4x4 MakeWorldMatrixFromInstance(in Input_Instance input)
{
	return float4x4(
		input.mat0, 
		input.mat1, 
		input.mat2, 
		float4(0, 0, 0, 1)
	);
}
inline float4x4 MakeWorldMatrixFromInstance(in Input_InstancePrev input)
{
	return float4x4(
		input.matPrev0,
		input.matPrev1,
		input.matPrev2,
		float4(0, 0, 0, 1)
		);
}

struct VertexSurface
{
	float4 position;
	float4 uvsets;
	float2 atlas;
	float4 color;
	float3 normal;
	float4 tangent;
	float4 positionPrev;

	inline void create(in ShaderMaterial material, in Input_Object_POS input)
	{
		position = float4(input.pos.xyz, 1);

		color = material.baseColor * unpack_rgba(input.inst.userdata.x);

		uint normal_wind = asuint(input.pos.w);
		normal.x = (float)((normal_wind >> 0) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.y = (float)((normal_wind >> 8) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.z = (float)((normal_wind >> 16) & 0xFF) / 255.0f * 2.0f - 1.0f;

		if (material.IsUsingWind())
		{
			const float windweight = ((normal_wind >> 24) & 0xFF) / 255.0f;
			const float waveoffset = dot(position.xyz, g_xFrame_WindDirection) * g_xFrame_WindWaveSize + (position.x + position.y + position.z) * g_xFrame_WindRandomness;
			const float3 wavedir = g_xFrame_WindDirection * windweight;
			const float3 wind = sin(g_xFrame_Time * g_xFrame_WindSpeed + waveoffset) * wavedir;
			position.xyz += wind;
		}
	}
	inline void create(in ShaderMaterial material, in Input_Object_POS_TEX input)
	{
		position = float4(input.pos.xyz, 1);

		color = material.baseColor * unpack_rgba(input.inst.userdata.x);

		uint normal_wind = asuint(input.pos.w);
		normal.x = (float)((normal_wind >> 0) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.y = (float)((normal_wind >> 8) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.z = (float)((normal_wind >> 16) & 0xFF) / 255.0f * 2.0f - 1.0f;

		if (material.IsUsingWind())
		{
			const float windweight = ((normal_wind >> 24) & 0xFF) / 255.0f;
			const float waveoffset = dot(position.xyz, g_xFrame_WindDirection) * g_xFrame_WindWaveSize + (position.x + position.y + position.z) * g_xFrame_WindRandomness;
			const float3 wavedir = g_xFrame_WindDirection * windweight;
			const float3 wind = sin(g_xFrame_Time * g_xFrame_WindSpeed + waveoffset) * wavedir;
			position.xyz += wind;
		}

		uvsets = float4(input.uv0 * material.texMulAdd.xy + material.texMulAdd.zw, input.uv1);

	}
	inline void create(in ShaderMaterial material, in Input_Object_ALL input)
	{
		position = float4(input.pos.xyz, 1);
		positionPrev = float4(input.pre.xyz, 1);

		color = material.baseColor * unpack_rgba(input.inst.userdata.x);

		if (material.IsUsingVertexColors())
		{
			color *= input.col;
		}

		uint normal_wind = asuint(input.pos.w);
		normal.x = (float)((normal_wind >> 0) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.y = (float)((normal_wind >> 8) & 0xFF) / 255.0f * 2.0f - 1.0f;
		normal.z = (float)((normal_wind >> 16) & 0xFF) / 255.0f * 2.0f - 1.0f;

		tangent = input.tan * 2 - 1;

		if (material.IsUsingWind())
		{
			const float windweight = ((normal_wind >> 24) & 0xFF) / 255.0f;
			const float waveoffset = dot(position.xyz, g_xFrame_WindDirection) * g_xFrame_WindWaveSize + (position.x + position.y + position.z) * g_xFrame_WindRandomness;
			const float waveoffsetPrev = dot(positionPrev.xyz, g_xFrame_WindDirection) * g_xFrame_WindWaveSize + (positionPrev.x + positionPrev.y + positionPrev.z) * g_xFrame_WindRandomness;
			const float3 wavedir = g_xFrame_WindDirection * windweight;
			const float3 wind = sin(g_xFrame_Time * g_xFrame_WindSpeed + waveoffset) * wavedir;
			const float3 windPrev = sin(g_xFrame_TimePrev * g_xFrame_WindSpeed + waveoffsetPrev) * wavedir;
			position.xyz += wind;
			positionPrev.xyz += windPrev;
		}

		uvsets = float4(input.uv0 * material.texMulAdd.xy + material.texMulAdd.zw, input.uv1);

		atlas = input.atl * input.instAtlas.atlasMulAdd.xy + input.instAtlas.atlasMulAdd.zw;
	}
};

#endif // WI_MESH_INPUT_LAYOUT_HF
