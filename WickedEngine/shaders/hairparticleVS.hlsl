#include "globals.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

RAWBUFFER(vertexBuffer_POS, 0);
RAWBUFFER(vertexBuffer_TEX, 1);
TYPEDBUFFER(primitiveBuffer, uint, 2);

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;
	Out.primitiveID = vid / 3;

	uint vertexID = primitiveBuffer[vid];
	uint4 data = vertexBuffer_POS.Load4(vertexID * 16);
	float3 position = asfloat(data.xyz);
	float3 normal = normalize(unpack_unitvector(data.w));
	float2 uv = unpack_half2(vertexBuffer_TEX.Load(vertexID * 4));

	Out.fade = saturate(distance(position.xyz, g_xCamera.CamPos.xyz) / xHairViewDistance);
	Out.fade = saturate(Out.fade - 0.8f) * 5.0f; // fade will be on edge and inwards 20%

	Out.pos = float4(position, 1);
	Out.pos3D = Out.pos.xyz;
	Out.pos = mul(g_xCamera.VP, Out.pos);

	Out.nor = normalize(normal);
	Out.tex = uv;

	return Out;
}
