#include "globals.hlsli"

float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return mul(pos, g_xTransform);
}