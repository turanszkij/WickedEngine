#include "globals.hlsli"

struct VertextoPixel{
	float4 pos				: SV_POSITION;
	float3 texPos			: TEXCOORD0;
	nointerpolation uint   sel				: TEXCOORD1;
	nointerpolation float4 opa				: TEXCOORD2;
};

Texture2D flare[7]:register(t1);


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color=0;
	[branch]
	switch(PSIn.sel){ // sad :(
		case 0:
			color=flare[0].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 1:
			color=flare[1].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 2:
			color=flare[2].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 3:
			color=flare[3].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 4:
			color=flare[4].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 5:
			color=flare[5].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		case 6:
			color=flare[6].SampleLevel(sampler_linear_clamp,PSIn.texPos.xy,0);
			break;
		default:break;
	};
	color*=1.1-saturate(PSIn.texPos.z);
	color*=PSIn.opa.x;
	return color;
}