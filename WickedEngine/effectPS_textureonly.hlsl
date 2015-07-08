#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "gammaHF.hlsli"
#include "toonHF.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = float4(0,0,0,1);

	baseColor = diffuseColor*(1-xFx.x);
	
	//if(PSIn.mat==matIndex){
		if(hasTex) {
			baseColor = xTextureTex.Sample(texSampler,PSIn.tex);
		}
		//baseColor=pow(baseColor,GAMMA);
	
		clip( baseColor.a < 0.1f ? -1:1 );

		//float light = saturate( dot(xSun,normalize(PSIn.nor)) );
		//if(toonshaded) toon(light);
		//light=clamp(light,xAmbient,1);
		//if(!shadeless.x) baseColor.rgb*=light*xSunColor;
		//baseColor=pow(baseColor,INV_GAMMA);
	//}
	//else discard;
	
	return baseColor*(1 + emit);
}

