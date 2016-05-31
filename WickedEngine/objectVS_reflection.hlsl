#include "objectHF.hlsli"
#include "objectHF_VS.hlsli"


PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;


	[branch]if ((uint)input.tex.z == g_xMatVS_matIndex)
	{

		float4x4 WORLD = MakeWorldMatrixFromInstance(input);

		Out.instanceColor = input.color_dither.rgb;
		Out.dither = input.color_dither.a;

		float4 pos = input.pos;
		float4 posPrev = input.pre;


		pos = mul(pos, WORLD);


		Out.clip = dot(pos, g_xClipPlane);

		float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
		affectWind(pos.xyz, input.tex.w, input.id);


		[branch]
		if (posPrev.w) {
			posPrev = mul(posPrev, WORLD);
		}
		else
			posPrev = pos;

		Out.pos = Out.pos2D = mul(pos, g_xCamera_ReflVP);
		Out.pos2DPrev = float4(0,0,0,0);
		Out.pos3D = pos.xyz;
		Out.tex = input.tex.xy * g_xMatVS_texMulAdd.xy + g_xMatVS_texMulAdd.zw;
		Out.nor = normalize(normal);
		Out.nor2D = float2(0,0);


		Out.ReflectionMapSamplingPos = float4(0,0,0,0);

		Out.ao = input.nor.w;

	}

	return Out;
}