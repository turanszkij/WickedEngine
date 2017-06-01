#ifndef _MESH_INPUT_LAYOUT_HF_
#define _MESH_INPUT_LAYOUT_HF_
#include "ResourceMapping.h"


struct Input_Instance
{
	float4 wi0;
	float4 wi1;
	float4 wi2;
	float4 color_dither;
};
struct Input_InstancePrev
{
	float4 wiPrev0;
	float4 wiPrev1;
	float4 wiPrev2;
};

struct Input_Shadow_POS
{
	float4 pos;
	Input_Instance instance;
};
struct Input_Shadow_POS_TEX
{
	float4 pos;
	float4 tex;
	Input_Instance instance;
};
struct Input_Skinning
{
	float4 pos;
	float4 nor;
	float4 bon;
	float4 wei;
};
struct Input_Object_POS
{
	float4 pos;
	Input_Instance instance;
};
struct Input_Object_POS_TEX
{
	float4 pos;
	float4 tex;
	Input_Instance instance;
};
struct Input_Object_ALL
{
	float4 pos;
	float4 nor;
	float4 tex;
	float4 pre;
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

#endif // _MESH_INPUT_LAYOUT_HF_