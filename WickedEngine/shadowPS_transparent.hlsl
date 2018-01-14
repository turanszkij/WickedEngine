#include "globals.hlsli"
#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float  dither : DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE_SIMPLE

	OBJECT_PS_SAMPLETEXTURES_SIMPLE

	OBJECT_PS_DEGAMMA

	//color.a = 1;

	OBJECT_PS_OUT_FORWARD_SIMPLE
}
