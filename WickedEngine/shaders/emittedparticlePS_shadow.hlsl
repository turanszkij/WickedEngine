#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "objectHF.hlsli"

float4 main(VertextoPixel input) : SV_Target
{
	ShaderMaterial material = EmitterGetMaterial();

	half4 color = 1;

	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		color = material.textures[BASECOLORMAP].Sample(sampler_linear_clamp, input.tex.xyxy);

		[branch]
		if (xEmitterOptions & EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED)
		{
			half4 color2 = material.textures[BASECOLORMAP].Sample(sampler_linear_clamp, input.tex.zwzw);
			color = lerp(color, color2, input.frameBlend);
		}
	}
	
	half4 inputColor;
	inputColor.r = ((input.color >> 0)  & 0xFF) / 255.0f;
	inputColor.g = ((input.color >> 8)  & 0xFF) / 255.0f;
	inputColor.b = ((input.color >> 16) & 0xFF) / 255.0f;
	inputColor.a = ((input.color >> 24) & 0xFF) / 255.0f;

	half opacity = color.a * inputColor.a;

	opacity = saturate(opacity);

	clip(opacity - 1.0 / 255.0);

	color.rgb = lerp(1, color.rgb, opacity);
	color.rgb *= 1 - opacity;

	color.a = input.pos.z; // secondary depth

	return color;
}
