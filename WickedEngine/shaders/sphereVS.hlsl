#include "globals.hlsli"
#include "uvsphere.hlsli"

struct VSOut_Sphere
{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float3 pos3D : TEXCOORD1;
};

VSOut_Sphere main( uint vID : SV_VERTEXID )
{
	VSOut_Sphere o;
	o.pos = UVSPHERE[vID];
	o.nor = o.pos.xyz;
	o.pos = mul(g_xTransform, o.pos);
	o.pos3D = o.pos.xyz;
	o.pos = mul(g_xCamera_VP, o.pos);
	return o;
}