float4x4 xProjection;
float4x4 xTrans;
float4	 xDimensions;

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertextoPixel main(float2 inPos : POSITION, float2 inTex : TEXCOORD0)
{
	VertextoPixel Out = (VertextoPixel)0;

	inPos.x-=xDimensions.x*0.5f;
	inPos.y+=xDimensions.y*0.5f;

	Out.pos = mul(float4(inPos,0,1),mul(xTrans,xProjection));
	
	Out.tex=inTex;

	return Out;
}
