#include "globals.hlsli"

TEXTURE3D(input, float4, 0);
RWTEXTURE3D(output, float4, 0);

SAMPLERSTATE(samplerstate, SSLOT_ONDEMAND0);

[numthreads(8, 8, 8)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint3 dim;
	output.GetDimensions(dim.x, dim.y, dim.z);

	if (DTid.x < dim.x && DTid.y < dim.y && DTid.z < dim.z)
	{
		output[DTid] = input.SampleLevel(samplerstate, ((float3)DTid + 0.5f) / (float3)dim, 0);
	}
}