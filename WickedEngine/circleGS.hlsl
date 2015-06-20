cbuffer constantbuffer:register(b0){
	float4x4 xWorld;
	float4x4 xView;
	float4x4 xProjection;
	float4   color;
}

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

static const int RES = 36;

[maxvertexcount((RES+1)*2)]
void main(
	point float4 input[1] : SV_POSITION, 
	inout TriangleStream< GSOutput > output
)
{
	float4x4 ViewProjection = mul(xView,xProjection);

	GSOutput center;
	center.col=color;
	center.pos = mul(mul(input[0],xWorld),ViewProjection);
	//output.Append(center);

	GSOutput element;
	element.col=color;

	for(uint i=0;i<=RES;++i)
	{
		float alpha = (float)i/(float)RES*2*3.14159265359;
		element.pos = input[0]+float4(sin(alpha),cos(alpha),0,0);
		element.pos = mul(mul(element.pos,xWorld),ViewProjection);
		output.Append(element);
		output.Append(center);
	}

	/*element.pos = input[0]+float4(-1,1,0,0);
	element.pos = mul(mul(element.pos,xWorld),ViewProjection);
	output.Append(element);
	
	element.pos = input[0]+float4(-1,-1,0,0);
	element.pos = mul(mul(element.pos,xWorld),ViewProjection);
	output.Append(element);
	
	element.pos = input[0]+float4(1,1,0,0);
	element.pos = mul(mul(element.pos,xWorld),ViewProjection);
	output.Append(element);
	
	element.pos = input[0]+float4(1,-1,0,0);
	element.pos = mul(mul(element.pos,xWorld),ViewProjection);
	output.Append(element);*/
}