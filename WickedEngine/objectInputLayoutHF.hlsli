#ifndef _MESH_INPUT_LAYOUT_HF_
#define _MESH_INPUT_LAYOUT_HF_

struct Input_Instance
{
	float4 wi0 : MATI0;
	float4 wi1 : MATI1;
	float4 wi2 : MATI2;
	float4 color_dither : COLOR_DITHER;
};
struct Input_InstancePrev
{
	float4 wiPrev0 : MATIPREV0;
	float4 wiPrev1 : MATIPREV1;
	float4 wiPrev2 : MATIPREV2;
};

struct Input_Object_POS
{
	float4 pos : POSITION_NORMAL_WIND;
	Input_Instance instance;
};
struct Input_Object_POS_TEX
{
	float4 pos : POSITION_NORMAL_WIND;
	float2 tex : TEXCOORD0;
	Input_Instance instance;
};
struct Input_Object_ALL
{
	float4 pos : POSITION_NORMAL_WIND;
	float2 tex : TEXCOORD0;
	float4 pre : TEXCOORD1;
	Input_Instance instance;
	Input_InstancePrev instancePrev;
};

inline float4x4 MakeWorldMatrixFromInstance(in Input_Instance input)
{
	return float4x4(
		  float4(input.wi0.x, input.wi1.x, input.wi2.x, 0)
		, float4(input.wi0.y, input.wi1.y, input.wi2.y, 0)
		, float4(input.wi0.z, input.wi1.z, input.wi2.z, 0)
		, float4(input.wi0.w, input.wi1.w, input.wi2.w, 1)
		);
}
inline float4x4 MakeWorldMatrixFromInstance(in Input_InstancePrev input)
{
	return float4x4(
		  float4(input.wiPrev0.x, input.wiPrev1.x, input.wiPrev2.x, 0)
		, float4(input.wiPrev0.y, input.wiPrev1.y, input.wiPrev2.y, 0)
		, float4(input.wiPrev0.z, input.wiPrev1.z, input.wiPrev2.z, 0)
		, float4(input.wiPrev0.w, input.wiPrev1.w, input.wiPrev2.w, 1)
		);
}

struct VertexSurface
{
	float4 position;
	float3 normal;
	float wind;
	float2 uv;
	float4 prevPos;
};
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_POS input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	uint normal_wind = asuint(input.pos.w);
	surface.normal.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.wind = (float)((normal_wind >> 24) & 0x000000FF) / 255.0f;

	return surface;
}
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_POS_TEX input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	uint normal_wind = asuint(input.pos.w);
	surface.normal.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.wind = (float)((normal_wind >> 24) & 0x000000FF) / 255.0f;

	surface.uv = input.tex.xy;

	return surface;
}
inline VertexSurface MakeVertexSurfaceFromInput(Input_Object_ALL input)
{
	VertexSurface surface;

	surface.position = float4(input.pos.xyz, 1);

	uint normal_wind = asuint(input.pos.w);
	surface.normal.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.normal.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	surface.wind = (float)((normal_wind >> 24) & 0x000000FF) / 255.0f;

	surface.uv = input.tex.xy;

	surface.prevPos = float4(input.pre.xyz, 1);

	return surface;
}

#endif // _MESH_INPUT_LAYOUT_HF_