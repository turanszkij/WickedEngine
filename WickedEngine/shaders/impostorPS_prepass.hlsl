#include "globals.hlsli"
#include "impostorHF.hlsli"

uint2 main(VSOut input) : SV_Target
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);
	float3 uv_col = float3(input.uv, input.slice);
	clip(impostorTex.Sample(sampler_linear_clamp, uv_col).a - 0.5f);

	PrimitiveID prim;
	prim.primitiveIndex = ~0u;
	prim.instanceIndex = input.instanceID;
	prim.subsetIndex = 0;
	return prim.pack();
}
