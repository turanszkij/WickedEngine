#include "globals.hlsli"
#include "grassHF_GS.hlsli"
#include "cullingShaderHF.hlsli"

static const uint vertexBuffer_stride = 16 * 3; // pos, normal, tangent 
RAWBUFFER(vertexBuffer, 0);

RWRAWBUFFER(argumentBuffer, 0);
RWRAWBUFFER(indexBuffer, 1);


[numthreads(GRASS_CULLING_THREADCOUNT, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint fetchAddress = DTid.x * vertexBuffer_stride;
	float4 pos = float4(asfloat(vertexBuffer.Load3(fetchAddress)), 1);
	pos = mul(pos, xWorld);
	float len = asfloat(vertexBuffer.Load(fetchAddress + 16 + 12));


	Sphere sphere = { mul(pos, g_xCamera_View).xyz, len };
	if (SphereInsideFrustum(sphere, frustum, 1, LOD2))
	{
		uint prevValue;
		argumentBuffer.InterlockedAdd(0, 1, prevValue);

		indexBuffer.Store(prevValue * 4, DTid.x);
	}
}