#include "globals.hlsli"

Texture2D<float4> xTextureTex:register(t0);
SamplerState texSampler:register(s0);

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

	[branch]if(g_xMat_hasTex)
		clip( xTextureTex.Sample(texSampler,PSIn.tex).a<0.1?-1:1 );

	return PSIn.pos.z;
}