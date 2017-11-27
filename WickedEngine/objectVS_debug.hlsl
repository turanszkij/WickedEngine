#include "globals.hlsli"

float4 main(float4 pos : POSITION_NORMAL_WIND) : SV_POSITION
{
	return mul(float4(pos.xyz, 1), g_xTransform);
}
