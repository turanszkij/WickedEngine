#include "skinningDEF.h"

struct Input{
	uint id: SV_VertexID;
	float4 pos : POSITION;
	float4 nor : NORMAL;
	float4 tex : TEXCOORD0;
	float4 pre : TEXCOORD1;
//#ifdef USE_GPU_SKINNING
//	float4 bon : TEXCOORD1;
//	float4 wei : TEXCOORD2;
//#endif
	float4 wi0 : MATI0;
	float4 wi1 : MATI1;
	float4 wi2 : MATI2;
	float4 color_dither : COLOR_DITHER;
};
