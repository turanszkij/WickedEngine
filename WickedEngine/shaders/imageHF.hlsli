#ifndef WI_IMAGE_HF
#define WI_IMAGE_HF
#include "globals.hlsli"
#include "ShaderInterop_Image.h"

// https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
float4 SampleBicubic(Texture2D tex, SamplerState linearSampler, float2 uv, float2 texSize)
{
    float2 samplePos = uv * texSize;
	float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

	float2 f = samplePos - texPos1;

	float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

	float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

	float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

	texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

	float4 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

	return result;
}

float Wedge2D(float2 v, float2 w)
{
	return v.x * w.y - v.y * w.x;
}

struct VertextoPixel
{
	float4 pos : SV_POSITION;
	float4 screen : TEXCOORD0;
	float2 q : TEXCOORD1;
	float2 edge : TEXCOORD2;

	float2 uv_screen()
	{
		return clipspace_to_uv(screen.xy / screen.w);
	}
	float4 compute_uvs()
	{
		float2 uv0;
		float2 uv1;

		[branch]
		if (image.flags & IMAGE_FLAG_FULLSCREEN)
		{
			uv0 = uv_screen();
			uv1 = uv0;
		}
		else
		{
			// Quad interpolation: http://reedbeta.com/blog/quadrilateral-interpolation-part-2/
			float2 b1 = image.b1;
			float2 b2 = image.b2;
			float2 b3 = image.b3;

			// Set up quadratic formula
			float A = Wedge2D(b2, b3);
			float B = Wedge2D(b3, q) - Wedge2D(b1, b2);
			float C = Wedge2D(b1, q);

			// Solve for v
			float2 uv;
			if (abs(A) < 0.001)
			{
				// Linear form
				uv.y = -C / B;
			}
			else
			{
				// Quadratic form. Take positive root for CCW winding with V-up
				float discrim = B * B - 4 * A * C;
				uv.y = 0.5 * (-B + sqrt(discrim)) / A;
			}

			// Solve for u, using largest-magnitude component
			float2 denom = b1 + uv.y * b3;
			if (abs(denom.x) > abs(denom.y))
				uv.x = (q.x - b2.x * uv.y) / denom.x;
			else
				uv.x = (q.y - b2.y * uv.y) / denom.y;

			uv0 = mad(uv, image.texMulAdd.xy, image.texMulAdd.zw);
			uv1 = mad(uv, image.texMulAdd2.xy, image.texMulAdd2.zw);
		}

		if (image.flags & IMAGE_FLAG_MIRROR)
		{
			uv0.x = 1 - uv0.x;
			uv1.x = 1 - uv1.x;
		}

		return float4(uv0, uv1);
	}
};

#endif // WI_IMAGE_HF

