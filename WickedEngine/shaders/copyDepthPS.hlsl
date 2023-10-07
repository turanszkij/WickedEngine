#include "globals.hlsli"

Texture2D<float> input_depth : register(t0);

float main(float4 pos : SV_Position) : SV_Depth
{
	return input_depth[uint2(pos.xy)];
}
