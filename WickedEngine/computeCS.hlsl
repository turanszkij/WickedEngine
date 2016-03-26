//struct CSBUFFER
//{
//	float x, y;
//};
//
//RWStructuredBuffer<CSBUFFER> BufferOut : register(u0);

RWTexture2D<float4> tex;

[numthreads(32, 32, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	tex[dispatchThreadId.xy] = float4((float2)dispatchThreadId.xy / 1024.0f, 0, 1);
}