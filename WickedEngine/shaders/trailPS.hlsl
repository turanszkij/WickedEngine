#include "globals.hlsli"

float4 main(float4 pos : SV_Position, float4 screen : SCREEN, float4 uv : TEXCOORD, float4 color : COLOR) : SV_TARGET
{
	color *= bindless_textures[descriptor_index(g_xTrailTextureIndex1)].Sample(sampler_linear_mirror, uv.xy);
	color *= bindless_textures[descriptor_index(g_xTrailTextureIndex2)].Sample(sampler_linear_mirror, uv.zw);
	
	[branch]
	if (g_xTrailLinearDepthTextureIndex >= 0)
	{
		Texture2D lineardepthTexture = bindless_textures[descriptor_index(g_xTrailLinearDepthTextureIndex)];
		float2 screenUV = clipspace_to_uv(screen.xy / screen.w);
		float depthScene = lineardepthTexture.SampleLevel(sampler_linear_clamp, screenUV, 0).r * g_xTrailCameraFar;
		float depthFragment = pos.w;
		color.a *= saturate(g_xTrailDepthSoften * (depthScene - depthFragment)); // soft depth fade
	}
	
	return color;
}
