#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"



float4 main(PixelInputType PSIn) : SV_TARGET
{
	float4 baseColor = float4(0,0,0,0);
	//uint mat = PSIn.mat;

	//if(mat == matIndex){
		if(hasTex) {
			baseColor = xTextureTex.Sample(texSampler,PSIn.tex);
			/*else if(mat==1) baseColor = xTextureTex1.Sample(texSampler,PSIn.tex);
			else if(mat==2) baseColor = xTextureTex2.Sample(texSampler,PSIn.tex);
			else if(mat==3) baseColor = xTextureTex3.Sample(texSampler,PSIn.tex);*/
		}
	
		clip( baseColor.a < 0.1f ? -1:1 );
	//}
	//else discard;

	return float4(baseColor.rgb*0,1);
}