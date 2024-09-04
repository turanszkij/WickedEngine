#include "globals.hlsli"

struct Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

float4 main(Input input) : SV_TARGET
{
	float4 color = 0;
	float3 P = input.pos3D;
	
	const float3 clipSpacePos = mul(g_xPaintDecalMatrix, float4(P, 1)).xyz;
	float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
	if (is_saturated(uvw))
	{
		Texture2D in_texture = bindless_textures[g_xPaintDecalTexture];
		color = in_texture.Sample(sampler_linear_clamp, uvw.xy);
		
		// blend out if close to cube Z:
		const half edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
		const half slopeBlend = g_xPaintDecalSlopeBlendPower > 0 ? pow(saturate(dot(input.normal, -get_forward(g_xPaintDecalMatrix))), g_xPaintDecalSlopeBlendPower) : 1;
		color.a *= edgeBlend * slopeBlend;
	}
	
	return color;
}
