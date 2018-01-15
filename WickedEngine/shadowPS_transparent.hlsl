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

	// When opacity reaches ZERO, the multiplicative light mask will be ONE:
	color.rgb = lerp(1, color.rgb, opacity);
	// When opacity reaches ONE, the light mask will be ZERO (fully occluding):
	color.rgb *= 1 - opacity;
	// And what goes in between will be tinted by the object color...

	// Use the alpha channel for something else (todo)
	color.a = 1;

	OBJECT_PS_OUT_FORWARD_SIMPLE
}
