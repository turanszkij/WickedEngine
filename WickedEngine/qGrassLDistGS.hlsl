#include "grassHF_GS.hlsli"
Texture2D tex:register(t0);

[maxvertexcount(4)]
void main(
	point VS_OUT input[1], 
	inout TriangleStream< QGS_OUT > output
)
{
	float2 frame;
    tex.GetDimensions(frame.x, frame.y);
	frame.xy/=frame.y;
	frame.x*=0.5f;
	
	uint rand = input[0].pos.w;
	float4x4 xViewProjection = mul(xView,xProjection);
	//float3 viewDir = xView._m02_m12_m22;
	float4 pos = float4(input[0].pos.xyz,1);
	float3 color = saturate(colTime.xyz+sin(pos.x-pos.y-pos.z)*0.013f)*0.5;
	float3 normal = /*normalize(*/input[0].nor.xyz/*-wind)*/;
#ifdef GRASS_FADE_DITHER
	float grassLength = /*lerp( */input[0].nor.w/*,0,pow(saturate(distance(pos.xyz,eye.xyz)/drawdistance),4) )*/;
#else
	float grassLength = lerp( input[0].nor.w,0,pow(saturate(distance(pos.xyz,eye.xyz)/drawdistance),4) );
#endif
	float3 wind = sin(colTime.w+(pos.x+pos.y+pos.z))*windDir.xyz*0.03*grassLength;
	//if(rand%(uint)windRandomness) wind=-wind;
	float3 front = xView._m02_m12_m22;
	float3 right = normalize(cross(front, normal));
	//if(input[0].vID%2) right*=-1;

	frame.xy*=grassLength;
	pos.xyz-=normal*0.1*grassLength;
	
	QGS_OUT element = (QGS_OUT)0;
	element.nor = normal;
#ifdef GRASS_FADE_DITHER
	element.fade = pow(saturate(distance(pos.xyz, eye.xyz) / drawdistance), 10);
#endif
	
	element.tex=float2(rand%2?1:0,0);
	element.pos=pos;
	element.pos.xyz+=-right*frame.x+normal*frame.y+wind;
	element.pos = mul(element.pos,xViewProjection);
	output.Append(element);

	element.tex=float2(rand%2?0:1,0);
	element.pos=pos;
	element.pos.xyz+=right*frame.x+normal*frame.y+wind;
	element.pos = mul(element.pos,xViewProjection);
	output.Append(element);

	element.tex=float2(rand%2?1:0,1);
	element.pos=pos;
	element.pos.xyz+=-right*frame.x;
	element.pos = mul(element.pos,xViewProjection);
	output.Append(element);
	
	element.tex=float2(rand%2?0:1,1);
	element.pos=pos;
	element.pos.xyz+=right*frame.x;
	element.pos = mul(element.pos,xViewProjection);
	output.Append(element);
}