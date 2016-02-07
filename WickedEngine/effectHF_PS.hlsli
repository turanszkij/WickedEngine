#ifndef _EFFECTHF_PS_
#define _EFFECTHF_PS_

//TextureCube enviroTex:register(t0);
//Texture2D<float4> xTextureRef:register(t1);
//Texture2D<float4> xTextureRefrac:register(t2);
//
//Texture2D<float4> xTextureTex:register(t3);
//Texture2D<float>  xTextureMat:register(t4);
//Texture2D<float4> xTextureNor:register(t5);
//Texture2D<float4> xTextureSpe:register(t6);

// texture_0	: texture map
// texture_1	: reflection map
// texture_2	: normal map
// texture_3	: specular map
// texture_4	: displacement map
// texture_5	: reflections
// texture_6	: refractions
// texture_7	: water ripples normal map
#define xTextureMap			texture_0
#define xReflectionMap		texture_1
#define xNormalMap			texture_2
#define xSpecularMap		texture_3
#define xDisplacementMap	texture_4
#define	xReflection			texture_5
#define xRefraction			texture_6
#define	xWaterRipples		texture_7

struct PixelOutputType
{
	float4 col	: SV_TARGET0;		// texture_gbuffer0
	float4 nor	: SV_TARGET1;		// texture_gbuffer1
	//float4 spe	: SV_TARGET2;
	float4 vel	: SV_TARGET2;		// texture_gbuffer2
};

#include "tangentComputeHF.hlsli"

#endif // _EFFECTHF_PS_
