#ifndef _MESH_INPUT_LAYOUT_HF_
#define _MESH_INPUT_LAYOUT_HF_

struct Input_Instance
{
	float4 wi0 : MATI0;
	float4 wi1 : MATI1;
	float4 wi2 : MATI2;
	float4 color_dither : COLOR_DITHER;
};
struct Input_Shadow
{
	uint id: SV_VertexID;
	float4 pos : POSITION;
	float4 tex : TEXCOORD0;
	Input_Instance instance;
};
struct Input_Skinning
{
	float4 pos : POSITION;
	float4 nor : NORMAL;
	float4 bon : TEXCOORD0;
	float4 wei : TEXCOORD1;
};
struct Input
{
	uint id: SV_VertexID;
	float4 pos : POSITION;
	float4 nor : NORMAL;
	float4 tex : TEXCOORD0;
	float4 pre : TEXCOORD1;
	Input_Instance instance;
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

#endif // _MESH_INPUT_LAYOUT_HF_