#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RWTEXTURE2D(resultTexture, float4, 0);

[numthreads(TRACEDRENDERING_CLEAR_BLOCKSIZE, TRACEDRENDERING_CLEAR_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	resultTexture[DTid.xy] = float4(0, 0, 0, 1);
}
