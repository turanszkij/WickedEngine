#include "globals.hlsli"


RWRAWBUFFER(argumentBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// create the default argument buffer at the beginning:
	argumentBuffer.Store(0, 0);		// IndexCountPerInstance
	argumentBuffer.Store(4, 1);		// InstanceCount
	argumentBuffer.Store(8, 0);		// StartIndexLocation
	argumentBuffer.Store(12, 0);	// BaseVertexLocation
	argumentBuffer.Store(16, 0);	// StartInstanceLocation
}