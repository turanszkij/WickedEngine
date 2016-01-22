#include "globals.hlsli"

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
	GSOutput center;
	center.col=g_xColor;
	center.pos = mul(mul(input[0], g_xTransform), g_xCamera_VP);

	GSOutput element;
	element.col=g_xColor;

	for(int i=0;i<=RES;++i)
	{
		float alpha = (float)i/(float)RES*2*3.14159265359;
		element.pos = input[0]+float4(sin(alpha),cos(alpha),0,0);
		element.pos = mul(mul(element.pos, g_xTransform), g_xCamera_VP);
		output.Append(element);
		output.Append(center);
	}
}