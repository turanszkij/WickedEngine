#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"

TEXTURE2D(sourceTexture, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(resultTexture, float4, 0);

[numthreads(RAYTRACING_ACCUMULATE_BLOCKSIZE, RAYTRACING_ACCUMULATE_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (xTraceAccumulationFactor == 1.0f)
	{
		resultTexture[DTid.xy] = sourceTexture[DTid.xy]; // naturally the lerp solution below would be enough, but if the result texture is not initialized, it can contain nan that doesn't work well with lerp!
	}
	else
	{
		resultTexture[DTid.xy] = lerp(resultTexture[DTid.xy], sourceTexture[DTid.xy], xTraceAccumulationFactor);
	}
}
