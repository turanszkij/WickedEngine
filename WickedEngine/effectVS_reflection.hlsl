#include "effectHF_PSVS.hlsli"
#include "effectHF_VS.hlsli"

Texture2D noiseTex:register(t0);

PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;


	[branch]if ((uint)input.tex.z == g_xMat_matIndex)
	{

		float4x4 WORLD = float4x4(
			float4(input.wi0.x, input.wi1.x, input.wi2.x, 0)
			, float4(input.wi0.y, input.wi1.y, input.wi2.y, 0)
			, float4(input.wi0.z, input.wi1.z, input.wi2.z, 0)
			, float4(input.wi0.w, input.wi1.w, input.wi2.w, 1)
			);

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
		Out.tex = input.tex.xy;
		Out.nor = normalize(normal);
		Out.nor2D = float3(0,0,0);


		Out.ReflectionMapSamplingPos = float4(0,0,0,0);

		Out.ao = input.nor.w;

	}

	return Out;
}