#include "grassHF_GS.hlsli"

[maxvertexcount(21)]
void main(
	point VS_OUT input[1], 
	inout TriangleStream< GS_OUT > output
)
{
	uint rand = input[0].pos.w;
	float4 pos = float4(input[0].pos.xyz,1);
	float3 color = saturate(xColor.xyz + sin(pos.x - pos.y - pos.z)*0.013f)*0.5;
	float3 wind = sin(g_xFrame_WindTime + (pos.x + pos.y + pos.z)*0.1f)*g_xFrame_WindDirection.xyz*0.1;
	if (rand % (uint)g_xFrame_WindRandomness) wind = -wind;
	float3 normal = /*normalize(*/input[0].nor.xyz/*-wind)*/;
	float length = /*lerp(*/input[0].nor.w/*,0,saturate(distance(pos.xyz,eye.xyz)/drawdistance))*/;
	//float3 front = cross(normal,float3(1,0,0))*0.5;
	float3 right = normalize(cross(input[0].tan.xyz, normal))*0.3;
	if(rand%2) right*=-1;
	//uint lod = input[0].pos.w;
	//color=float3(0,0.8,0);
	
	for(uint i=0;i<3;++i){
		float4 mod = pos + float4( cross( MOD[i],normal),0 );
		genBlade(output,mod,normal,length,2,right,color, wind, 0);
	}

}