#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

RWStructuredBuffer<uint> rayallocationBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint rayCount = rayallocationBuffer[0];
	rayallocationBuffer[0] = (rayCount + 31) / 32;
	rayallocationBuffer[1] = 1;
	rayallocationBuffer[2] = 1;
	rayallocationBuffer[3] = rayCount;
}
