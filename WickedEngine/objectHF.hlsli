#ifndef _OBJECTSHADER_HF_
#define _OBJECTSHADER_HF_
#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"
#include "ditherHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "specularHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "globalsHF.hlsli"
#include "dirLightHF.hlsli"

// DEFINITIONS
//////////////////

//#define xTextureMap			texture_0
//#define xReflectionMap		texture_1
//#define xNormalMap			texture_2
//#define xSpecularMap		texture_3
//#define xDisplacementMap	texture_4
//#define	xReflection			texture_5

#define xBaseColorMap		texture_0
#define xNormalMap			texture_1
#define xRoughnessMap		texture_2
#define xReflectanceMap		texture_3
#define xMetalnessMap		texture_4
#define xDisplacementMap	texture_0

#define xReflection			texture_5
#define xRefraction			texture_6
#define	xWaterRipples		texture_7

struct PixelInputType
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	float3 nor								: NORMAL;
	float4 pos2D							: SCREENPOSITION;
	float3 pos3D							: WORLDPOSITION;
	float4 pos2DPrev						: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos			: TEXCOORD1;
	float  ao								: AMBIENT_OCCLUSION;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float2 nor2D							: NORMAL2D;
};


#endif // _OBJECTSHADER_HF_