#ifndef WI_IMAGE_HF
#define WI_IMAGE_HF
#include "globals.hlsli"
#include "ShaderInterop_Image.h"

TEXTURE2D(texture_base, float4, TEXSLOT_IMAGE_BASE);
TEXTURE2D(texture_mask, float4, TEXSLOT_IMAGE_MASK);
TEXTURE2D(texture_background, float4, TEXSLOT_IMAGE_BACKGROUND);

SAMPLERSTATE(Sampler, SSLOT_ONDEMAND0);

float Wedge2D(float2 v, float2 w)
{
	return v.x * w.y - v.y * w.x;
}

struct VertextoPixel
{
	float4 pos : SV_POSITION;
	float2 q : TEXCOORD3;
	float2 b1 : TEXCOORD4;
	float2 b2 : TEXCOORD5;
	float2 b3 : TEXCOORD6;
	float4 uv_screen : TEXCOORD2;

	float4 compute_uvs()
	{
		// Quad interpolation: http://reedbeta.com/blog/quadrilateral-interpolation-part-2/

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

		float2 uv0 = uv * xTexMulAdd.xy + xTexMulAdd.zw;
		float2 uv1 = uv * xTexMulAdd2.xy + xTexMulAdd2.zw;
		return float4(uv0, uv1);
	}
};

#endif // WI_IMAGE_HF

