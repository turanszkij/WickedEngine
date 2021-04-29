#include "globals.hlsli"

TEXTURE2D(texture_0, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_1, float4, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_2, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(texture_3, float4, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_4, float4, TEXSLOT_ONDEMAND4);
TEXTURE2D(texture_5, float4, TEXSLOT_ONDEMAND5);
TEXTURE2D(texture_6, float4, TEXSLOT_ONDEMAND6);

struct VertextoPixel{
	float4 pos					: SV_POSITION;
	float3 texPos				: TEXCOORD0;
	nointerpolation uint   sel	: TEXCOORD1;
	nointerpolation float4 opa	: TEXCOORD2;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color=0;
	
	// todo: texture atlas or array
	[branch]
	switch(PSIn.sel)
	{
		case 0:
			color = texture_0.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 1:
			color = texture_1.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 2:
			color = texture_2.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 3:
			color = texture_3.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 4:
			color = texture_4.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 5:
			color = texture_5.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		case 6:
			color = texture_6.SampleLevel(sampler_linear_clamp, PSIn.texPos.xy, 0);
			break;
		default:break;
	};

	color *= 1.1 - saturate(PSIn.texPos.z);
	color *= PSIn.opa.x;

	return color;
}