#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

RWStructuredBuffer<IndirectDrawArgsInstanced> indirectBuffer : register(u0);

struct Push
{
	uint camera_count;
};
PUSHCONSTANT(push, Push);

[numthreads(1, 1, 1)]
void main()
{
	indirectBuffer[0].InstanceCount *= push.camera_count;
}
