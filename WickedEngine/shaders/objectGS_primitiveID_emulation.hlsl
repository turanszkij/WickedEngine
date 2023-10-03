#include "globals.hlsli"
// This geometry shader is intended as fallback support when GPU doesn't support SV_PrimitiveID input to pixel shader

struct GSInput
{
	float4 pos : SV_POSITION;
	float clip : SV_ClipDistance0;
	uint instanceIndex_dither : INSTANCEINDEX_DITHER;
#ifdef ALPHATEST
	float4 uvsets : UVSETS;
#endif // ALPHATEST
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float clip : SV_ClipDistance0;
	uint instanceIndex_dither : INSTANCEINDEX_DITHER;
#ifdef ALPHATEST
	float4 uvsets : UVSETS;
#endif // ALPHATEST
	uint primitiveID : PRIMITIVEID;
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	in uint primitiveID : SV_PrimitiveID,
	inout TriangleStream<GSOutput> output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GSOutput element;
		element.pos = input[i].pos;
		element.clip = input[i].clip;
		element.instanceIndex_dither = input[i].instanceIndex_dither;
#ifdef ALPHATEST
		element.uvsets = input[i].uvsets;
#endif // ALPHATEST
		element.primitiveID = primitiveID;
		output.Append(element);
	}
}
