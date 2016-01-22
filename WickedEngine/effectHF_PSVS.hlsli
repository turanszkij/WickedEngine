#ifndef _EFFECTHF_PSVS_
#define _EFFECTHF_PSVS_
#include "globals.hlsli"

struct PixelInputType
{
	float4 pos						: SV_POSITION;
	float clip						: SV_ClipDistance0;
	float2 tex						: TEXCOORD0;
	float3 nor						: NORMAL;
	float4 pos2D					: SCREENPOSITION;
	float3 pos3D					: WORLDPOSITION;
	float4 pos2DPrev				: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos : TEXCOORD1;
	float  ao						: AMBIENT_OCCLUSION;
	float  dither					: DITHER;
	float3 instanceColor			: INSTANCECOLOR;
	float3 nor2D					: NORMAL2D;
};

#endif // _EFFECTHF_PSVS_
