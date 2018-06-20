#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	output[DTid.xy + xCopyDest] = input[DTid.xy];
}
