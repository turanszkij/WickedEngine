#include "globals.hlsli"
#include "icosphere.hlsli"
#include "skyHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float2 clipspace : TEXCOORD;
};

VSOut main(uint vI : SV_VERTEXID)
{
	VSOut Out;

	FullScreenTriangle(vI, Out.pos);

	Out.clipspace = Out.pos.xy;

	return Out;
}
