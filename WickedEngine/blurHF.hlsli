#include "imageHF.hlsli"

cbuffer constants:register(b0){
	float4 weight;
	float4 weightTexelStrenMip;
}

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};