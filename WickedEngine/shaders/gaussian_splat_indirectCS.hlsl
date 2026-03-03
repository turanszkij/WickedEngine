#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

RWStructuredBuffer<IndirectDrawArgsInstanced> indirectBuffer : register(u0);

[numthreads(1, 1, 1)]
void main()
{
	indirectBuffer[0].InstanceCount *= cb.cameraCount;
}
