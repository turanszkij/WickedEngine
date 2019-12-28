#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

#define DOF_DEPTH_SCALE_FOREGROUND (g_xCamera_ZFarP * 0.25f)
float2 DepthCmp2(float depth, float closestTileDepth)
{
	float d = DOF_DEPTH_SCALE_FOREGROUND * (depth - closestTileDepth);
	float2 depthCmp;
	depthCmp.x = smoothstep(0.0, 1.0, d); // Background
	depthCmp.y = 1.0 - depthCmp.x; // Foreground
	return depthCmp;

}

#define DOF_SINGLE_PIXEL_RADIUS 0.7071 // length( float2(0.5, 0.5 ) )
float SampleAlpha(float sampleCoc) 
{ 
	return min(rcp(PI * sampleCoc * sampleCoc), rcp(PI * DOF_SINGLE_PIXEL_RADIUS * DOF_SINGLE_PIXEL_RADIUS)); 
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float mindepth = neighborhood_mindepth_maxcoc[DTid.xy / DEPTHOFFIELD_TILESIZE].x;
	const float depth = texture_lineardepth[DTid.xy];
	const float coc = dof_scale * abs(1 - dof_focus / (depth * g_xCamera_ZFarP));
	const float alpha = SampleAlpha(coc);
	const float2 depthcmp = DepthCmp2(depth, mindepth);

	output[DTid.xy] = float4(coc, alpha * depthcmp.x, alpha * depthcmp.y, 1);
}
