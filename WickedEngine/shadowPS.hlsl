Texture2D<float4> xTextureTex:register(t0);
SamplerState texSampler:register(s0);

cbuffer matBuffer:register(b1){
	float4 diffuseColor;
	uint hasRef,hasNor,hasTex,hasSpe;
	float4 specular;
	float refractionIndex;
	float2 movingTex;
	float metallic;
	uint shadeless;
	uint specular_power;
	uint toonshaded;
	uint matIndex;
	float emit;
	float padding[3];
};

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float main(VertextoPixel PSIn) : SV_TARGET
{
	//float4 color = PSIn.pos.z/PSIn.pos.w;
	/*float4 color = diffuseColor;

	if(PSIn.mat==matIndex){
		if(hasRefNorTexSpe.z)
			color = xTextureTex.Sample(texSampler,PSIn.tex);
	
		clip( color.a < 0.1f ? -1:1 );
	}
	else discard;

	return (color.rgb,1);*/
	//return xTextureTex.Sample(texSampler,PSIn.tex);

	[branch]if(hasTex)
		clip( xTextureTex.Sample(texSampler,PSIn.tex).a<0.1?-1:1 );

	return PSIn.pos.z;
}