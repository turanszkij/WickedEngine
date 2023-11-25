#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

uint main(VSOut input, out uint coverage : SV_Coverage) : SV_Target
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);
	float3 uv_col = float3(input.uv, input.slice);
	float alpha = impostorTex.Sample(sampler_linear_clamp, uv_col).a;
	coverage = AlphaToCoverage(alpha, 0.75, input.pos);

	PrimitiveID prim;
	prim.primitiveIndex = input.primitiveID;
	prim.instanceIndex = GetScene().impostorInstanceOffset;
	prim.subsetIndex = 0;
	return prim.pack();
}
