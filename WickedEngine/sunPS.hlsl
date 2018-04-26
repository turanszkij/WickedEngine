#include "objectHF.hlsli"
#include "globals.hlsli"
#include "skyHF.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

float4 main(VSOut PSIn) : SV_TARGET
{ 
	float3 normal = normalize(PSIn.nor);


#ifdef SHADERCOMPILER_SPIRV
	//compiler bug workaround:
	uint ucol = EntityArray[g_xFrame_SunEntityArrayIndex].color;
	float3 sunc;

	sunc.x = (float)((ucol >> 0) & 0x000000FF) / 255.0f;
	sunc.y = (float)((ucol >> 8) & 0x000000FF) / 255.0f;
	sunc.z = (float)((ucol >> 16) & 0x000000FF) / 255.0f;
#else
	float3 sunc = GetSunColor();
#endif

	float4 color = float4(normal.y > 0 ? max(pow(saturate(dot(GetSunDirection().xyz, normal) + 0.0001), 64)*sunc, 0) : 0, 1);

	AddCloudLayer(color, normal, true);

	return color;
}