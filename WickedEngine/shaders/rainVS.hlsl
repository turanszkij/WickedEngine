#include "globals.hlsli"
#include "cylinder.hlsli"

struct PushConstants
{
	uint layers;
};
PUSHCONSTANT(push, PushConstants);

void main(
	in uint vertexID : SV_VertexID,
	in uint layer : SV_InstanceID,
	out float4 pos : SV_Position,
	out float3 P : TEXCOORD1,
	out float3 uvw : TEXCOORD2
)
{
	float near_layer = 1;
	float far_layer = 100;
	float t = 1 - saturate(float(layer) / float(push.layers));
	float layer_distance = lerp(near_layer, far_layer, pow(t, 2));

    float fSinAngle;
    float fCosAngle;
    sincos(PI * 0.5, fSinAngle, fCosAngle);

    float3x3 rotZ = float3x3(
	   fCosAngle, fSinAngle, 0,
	   -fSinAngle, fCosAngle, 0,
	   0, 0, 1
	);
	
    sincos(hash1(layer) * PI * 2, fSinAngle, fCosAngle);
	float2x2 rotY = float2x2(
		fCosAngle, -fSinAngle,
		fSinAngle, fCosAngle
	);
	
	pos = float4(CYLINDER[vertexID].xyz, 1);
	pos.xyz = mul(pos.xyz, rotZ);
	float3 V = pos.xyz;
	V.xz = mul(V.xz, rotY);
	//pos.y *= GetCamera().z_far;
	pos.y *= 5000;
	uvw = pos.xyz;
	uvw.xz *= layer_distance;
	pos.xz *= layer_distance * lerp(0.8, 1.2, sin(dot(float2(0,1), V.xz) * 4 + GetTime()) * 0.5 + 0.5);
	pos.xyz += GetCamera().position;
	P = pos.xyz;
	pos = mul(GetCamera().view_projection, pos);
}
