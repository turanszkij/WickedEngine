cbuffer matrixBuffer:register(b0){
	float4x4 xWorldViewProjection;
	float4 color;
};

struct VertexToPixel{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR;
};

VertexToPixel main( float3 inPos : POSITION )
{
	VertexToPixel Out = (VertexToPixel)0;
	Out.pos = mul( float4(inPos,1), xWorldViewProjection );
	Out.col = color;
	return Out;
}