#include "objectHF.hlsli"



float4 main(Input_Object_POS input) : SV_POSITION
{
	PixelInputType_Simple Out = (PixelInputType_Simple)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	float4 pos = float4(input.pos.xyz, 1);

	pos = mul(pos, WORLD);

	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);


	return mul(pos, g_xCamera_VP);
}
