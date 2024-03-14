#include "globals.hlsli"

float4 main(float4 pos : SV_Position, float4 uv : TEXCOORD, float4 color : COLOR) : SV_TARGET
{
	color *= bindless_textures[g_xTrailTextureIndex1].Sample(sampler_linear_mirror, uv.xy);
	color *= bindless_textures[g_xTrailTextureIndex2].Sample(sampler_linear_mirror, uv.zw);
	
	[branch]
	if (g_xTrailLinearDepthTextureIndex >= 0)
	{
		Texture2D lineardepthTexture = bindless_textures[g_xTrailLinearDepthTextureIndex];
		float depthScene = lineardepthTexture[pos.xy].r * g_xTrailCameraFar;
		float depthFragment = pos.w;
		color.a *= saturate(g_xTrailDepthSoften * (depthScene - depthFragment)); // soft depth fade
	}
	
	return color;
}
