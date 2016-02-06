#include "globals.hlsli"
#include "effectInputLayoutHF.hlsli"

struct GS_CUBEMAP_IN 
{ 
	float4 Pos		: SV_POSITION;    // World position 
    float2 Tex		: TEXCOORD0;      // Texture coord
}; 

GS_CUBEMAP_IN main(Input input)
{
	GS_CUBEMAP_IN Out = (GS_CUBEMAP_IN)0;

	[branch]
	if((uint)input.tex.z == g_xMat_matIndex)
	{
		

		float4x4 WORLD = MakeWorldMatrixFromInstance(input);

		float4 pos = input.pos;
		
//#ifdef SKINNING_ON
//		Skinning(pos,input.bon,input.wei);
//#endif

		Out.Pos = mul(pos,WORLD);
		Out.Tex = input.tex.xy;

	}

	return Out;
}