#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "gammaHF.hlsli"
#include "toonHF.hlsli"
#include "specularHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "ditherHF.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	clip(dither(PSIn.pos.xy) - PSIn.dither);

	float4 baseColor = float4(0, 0, 0, 1);
	float depth = PSIn.pos2D.z;

	if (g_xMat_hasTex) {
		baseColor = xTextureTex.Sample(texSampler, PSIn.tex);
	}
	baseColor.rgb *= PSIn.instanceColor;

	clip(baseColor.a < 0.1f ? -1 : 1);

	if (!g_xMat_shadeless.x)
	{
		float4 spec = g_xMat_specular;
		float3 normal = normalize(PSIn.nor);
		float3 eyevector = normalize(g_xCamera_CamPos - PSIn.pos3D);

		baseColor = pow(abs(baseColor), GAMMA);
		//NORMALMAP
		float3 bumpColor = 0;
		if (g_xMat_hasNor){
			float4 nortex = xTextureNor.Sample(texSampler, PSIn.tex + g_xMat_movingTex);
				if (nortex.a>0){
					float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex.xy);
						bumpColor = 2.0f * nortex.rgb - 1.0f;
					//bumpColor.g*=-1;
					normal = normalize(mul(bumpColor, tangentFrame));
				}
		}

		spec = lerp(spec, xTextureSpe.Sample(texSampler, PSIn.tex).r, g_xMat_hasSpe);

		//ENVIROMENT MAP
		float4 envCol = 0;
		{
			uint mip = 0;
			float2 size;
			float mipLevels;
			enviroTex.GetDimensions(mip, size.x, size.y, mipLevels);

			float3 ref = normalize(reflect(-eyevector, normal));
				envCol = enviroTex.SampleLevel(texSampler, ref, (1 - smoothstep(0, 128, g_xMat_specular_power))*mipLevels);
			baseColor = lerp(baseColor, envCol, g_xMat_metallic*spec);
		}

		float3 light = saturate( dot(g_xWorld_SunDir.xyz,normalize(PSIn.nor)) );
		light=clamp(light, g_xWorld_Ambient.rgb,1);
		baseColor.rgb *= light*g_xWorld_SunColor.rgb;

		applySpecular(baseColor, g_xWorld_SunColor, normal, eyevector, g_xWorld_SunDir.xyz, 1, g_xMat_specular_power, spec.a, 0);

		baseColor = pow(abs(baseColor*(1 + g_xMat_emit)), INV_GAMMA);

		baseColor.rgb = applyFog(baseColor.rgb, getFog(getLinearDepth(depth / PSIn.pos2D.w)));
	}

	return baseColor;
}

