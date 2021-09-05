#include "globals.hlsli"
#include "impostorHF.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.dither);

	float3 uv_col = float3(input.uv, input.slice);

	ShaderMeshInstance instance = load_instance(input.instanceID);
	float4 color = unpack_rgba(instance.color);

	return color;
}
