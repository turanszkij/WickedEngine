#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_main, float3, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output_postfilter, float3, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 color = 0;
	color += texture_main[DTid.xy + int2(-1, -1)];
	color += texture_main[DTid.xy + int2(0, -1)];
	color += texture_main[DTid.xy + int2(1, -1)];
	color += texture_main[DTid.xy + int2(-1, 0)];
	color += texture_main[DTid.xy + int2(0, 0)];
	color += texture_main[DTid.xy + int2(1, 0)];
	color += texture_main[DTid.xy + int2(-1, 1)];
	color += texture_main[DTid.xy + int2(0, 1)];
	color += texture_main[DTid.xy + int2(1, 1)];
	color /= 9.0f;

	output_postfilter[DTid.xy] = color;
	//output_postfilter[DTid.xy] = texture_main[DTid.xy];
}