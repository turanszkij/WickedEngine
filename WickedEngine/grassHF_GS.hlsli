#ifndef GRASSHF_GS
#define GRASSHF_GS
#include "globals.hlsli"

//undef for length fade instead of dithering
#define GRASS_FADE_DITHER

CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float3 xColor;
	float xDrawDistance;
}


struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 nor : NORMAL;
	float4 tan : TANGENT;
};
struct GS_OUT
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float3 col : COLOR;
	float  fade : DITHERFADE;
};
struct QGS_OUT
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float  fade : DITHERFADE;
	float4 pos2D : SCREENPOSITION;
};

static const float3 MOD[] = {
	float3(-0.010735935, 0.01647018, 0.0062425877),
	float3(-0.06533369, 0.3647007, -0.13746321),
	float3(-0.6539235, -0.016726388, -0.53000957),
	float3(0.40958285, 0.0052428036, -0.5591124),
	float3(-0.1465366, 0.09899267, 0.15571679),
	float3(-0.44122112, -0.5458797, 0.04912532),
};

inline void genBlade(inout TriangleStream< GS_OUT > output, float4 pos, float3 normal
					 , float length,in float width, float3 right, float3 color,in float3 wind,in float fade)
{
	//float3 right = cross(front,normal)*0.3;
	GS_OUT element = (GS_OUT)0;
	element.nor = normal;
	element.fade = fade;

	width*=0.3;
	wind*=length*0.1;
	right*=width;
	normal*=length;

	[branch]if(length>2){
	
		element.pos = pos;
		element.pos.xyz += -right*0.5;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = color;
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += right*0.5;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += -right*0.4+normal*0.3+wind*0.2;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.2);
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += right*0.4+normal*0.3+wind*0.2;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += -right*0.2+normal*0.7+wind*1.2;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.6);
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += right*0.2+normal*0.7+wind*1.2;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		output.Append(element);
	
		element.pos = pos;
		element.pos.xyz += normal+wind*2.6;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.9);
		output.Append(element);

	}
	else{
		float3 front = cross(normal,right);

		element.pos = pos;
		element.pos.xyz += -right*2.3+normal*0.56+wind*2+front;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.9);
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += -right+normal*0.4+wind+front;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.3);
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += -right+front*0.5;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = color;
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += right;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += right+normal*0.6+wind*1.6;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.5);
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += right*2+normal*0.6+wind*1.6;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.7);
		output.Append(element);
		
		element.pos = pos;
		element.pos.xyz += right*3.4+normal+wind*2.8;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = mul(element.pos,g_xCamera_VP);
		element.col = saturate(color*1.9);
		output.Append(element);
	}

	output.RestartStrip();
}

#endif //GRASSHF_GS

