Texture2D<float4> xTextureTex:register(t0);
SamplerState texSampler:register(s0);

cbuffer matBuffer:register(b1){
	float4 diffuseColor;
	float4 hasRefNorTexSpe;
	float4 specular;
	float4 refractionIndexMovingTexEnv;
};
cbuffer lightBuffer:register(b2){
	float4 lightPos;
	float4 lightColor;
	float4 lightEnerdis;
};

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float3 pos3D			: POSITION3D;
	float2 tex				: TEXCOORD0;
    uint RTIndex			: SV_RenderTargetArrayIndex;
};

float main(VertextoPixel PSIn) : SV_DEPTH
{
	[branch]if(hasRefNorTexSpe.z)
		clip( xTextureTex.Sample(texSampler,PSIn.tex).a<0.1?-1:1 );
	return distance(PSIn.pos3D,lightPos)/lightEnerdis.y;
}