struct VertextoPixel{
	float4 pos				: SV_POSITION;
	float3 texPos			: TEXCOORD0;
	uint   sel				: TEXCOORD1;
	float4 opa				: TEXCOORD2;
};

Texture2D flare[7]:register(t1);
SamplerState flareSampler:register(s0);


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	//float4 color = flare[0].Sample(flareSampler,PSIn.texPosSel.xy);
	//color = lerp(color,flare[1].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/1));
	//color = lerp(color,flare[2].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/2));
	//color = lerp(color,flare[3].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/3));
	//color = lerp(color,flare[4].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/4));
	//color = lerp(color,flare[5].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/5));
	//color = lerp(color,flare[6].Sample(flareSampler,PSIn.texPosSel.xy), int(PSIn.texPosSel.w/6));
	float4 color=0;
	[branch]
	switch(PSIn.sel){ // sad :(
		case 0:
			color=flare[0].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 1:
			color=flare[1].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 2:
			color=flare[2].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 3:
			color=flare[3].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 4:
			color=flare[4].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 5:
			color=flare[5].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		case 6:
			color=flare[6].SampleLevel(flareSampler,PSIn.texPos.xy,0);
			break;
		default:break;
	};
	color*=1.1-saturate(PSIn.texPos.z);
	color*=PSIn.opa.x;
	return color;
}