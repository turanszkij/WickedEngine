#include "globals.hlsli"

//Texture2D<float4> xRefracTexture:register(t0);
//Texture2D<float4> xDistortionTexture:register(t1);
//Texture2D<float4> xTexture:register(t2);

// texture_0 : refraction map
// texture_1 : distortion map
// texture_2 : texture map

struct VertexToPixel{
	float4 pos						: SV_POSITION;
	float2 tex						: TEXCOORD0;
	float4 col						: TEXCOORD1;
	float4 dis						: TEXCOORD2;
};

float4 main(VertexToPixel PSIn) : SV_TARGET
{

	float2 co = abs(PSIn.tex - 0.5f)*2.f;
	float blendOut = 1 - pow(max(co.x,co.y), 2);
	blendOut *= PSIn.col.a;

	float4 tex = texture_2.Sample(sampler_linear_mirror,PSIn.tex);
	tex.a *= blendOut;

	float4 color = float4(PSIn.col.rgb * blendOut,1);

	float2 distortionCo;
		distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
		distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
	float2 distort = (texture_1.Sample(sampler_linear_mirror,PSIn.tex).rg - float2(0.5f, 0.5f))*0.3f * blendOut;

	// Chromatic Aberration
	float2 dim;
	texture_0.GetDimensions(dim.x, dim.y);
	dim = 4.0f / dim * blendOut;
	color.r += texture_0.SampleLevel(sampler_linear_mirror, distortionCo + distort + dim * float2(1, 1), 0).r;
	color.g += texture_0.SampleLevel(sampler_linear_mirror, distortionCo + distort + dim * float2(-1, 1), 0).g;
	color.b += texture_0.SampleLevel(sampler_linear_mirror, distortionCo + distort + dim * float2(0, -1), 0).b;

	color.rgb = lerp(color.rgb, tex.rgb, tex.a);

	return float4(color.rgb,1);
}