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

	baseColor = diffuseColor*(1 - xFx.x);

	if (hasTex) {
		baseColor = xTextureTex.Sample(texSampler, PSIn.tex);
	}

	clip(baseColor.a < 0.1f ? -1 : 1);

	if (!shadeless.x)
	{
		float4 spec = specular;
		float3 normal = normalize(PSIn.nor);
		float3 eyevector = normalize(PSIn.cam - PSIn.pos3D);

		baseColor = pow(baseColor, GAMMA);
		//NORMALMAP
		float3 bumpColor = 0;
		if (hasNor){
			float4 nortex = xTextureNor.Sample(texSampler, PSIn.tex + movingTex);
				if (nortex.a>0){
					float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex.xy);
						bumpColor = 2.0f * nortex - 1.0f;
					//bumpColor.g*=-1;
					normal = normalize(mul(bumpColor, tangentFrame));
				}
		}

		spec = lerp(spec, xTextureSpe.Sample(texSampler, PSIn.tex).r, hasSpe);

		//ENVIROMENT MAP
		float4 envCol = 0;
		{
			uint mip = 0;
			float2 size;
			float mipLevels;
			enviroTex.GetDimensions(mip, size.x, size.y, mipLevels);

			float3 ref = normalize(reflect(-eyevector, normal));
				envCol = enviroTex.SampleLevel(texSampler, ref, (1 - smoothstep(0, 128, specular_power))*mipLevels);
			baseColor = lerp(baseColor, envCol, metallic*spec);
		}

		float light = saturate( dot(xSun,normalize(PSIn.nor)) );
		light=clamp(light,xAmbient,1);
		baseColor.rgb *= light*xSunColor;

		applySpecular(baseColor, xSunColor, normal, eyevector, xSun, 1, specular_power, spec.a, 0);

		baseColor = pow(baseColor*(1 + emit), INV_GAMMA);

		baseColor.rgb = applyFog(baseColor.rgb, xHorizon, getFog(getLinearDepth(depth / PSIn.pos2D.w), xFogSEH));
	}

	return baseColor;
}

