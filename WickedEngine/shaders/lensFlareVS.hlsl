#include "globals.hlsli"

TEXTURE2D(texture_occlusion, float4, TEXSLOT_ONDEMAND0);

static const float2 BILLBOARD[] = {
	float2(-1, -1),
	float2(1, -1),
	float2(-1, 1),
	float2(1, 1),
};

struct VertexOut
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	nointerpolation float opacity : TEXCOORD1;
};

VertexOut main(uint vertexID : SV_VertexID)
{
	VertexOut Out;
	Out.uv = BILLBOARD[vertexID] * 0.5f + 0.5f;

	// determine the flare opacity based on depth buffer occlusion and occlusion mask:
	float referenceDepth = saturate(1 - xLensFlarePos.z);
	const float2 step = 1.0 / (GetInternalResolution() * xLensFlarePos.z);
	const float2 range = 10.5 * step;
	float samples = 0;
	float visibility = 0;
	for (float y = -range.y; y <= range.y; y += step.y)
	{
		for (float x = -range.x; x <= range.x; x += step.x)
		{
			samples++;
			float vis = texture_occlusion.SampleLevel(sampler_linear_clamp, xLensFlarePos.xy + float2(x, y), 0).r;
			vis *= texture_depth.SampleLevel(sampler_point_clamp, xLensFlarePos.xy + float2(x, y), 0).r <= referenceDepth ? 1 : 0;
			visibility += vis;
		}
	}
	visibility /= samples;

	Out.opacity = visibility;

	float2 pos = (xLensFlarePos.xy - 0.5) * float2(2, -2);
	float2 moddedpos = pos * xLensFlareOffset;
	Out.opacity *= saturate(1 - length(pos - moddedpos));

	Out.pos = float4(moddedpos + BILLBOARD[vertexID] * xLensFlareSize * g_xFrame_CanvasSize_rcp, 0, 1);

	return Out;
}
