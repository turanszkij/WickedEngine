#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

struct ImageCB
{
	float4 xCorners[4];
	float4 xTexMulAdd;
	float4 xTexMulAdd2;
	float4 xColor;
};
//ROOTCONSTANTS(imageCB, ImageCB, CBSLOT_IMAGE);
CONSTANTBUFFER(imageCB, ImageCB, CBSLOT_IMAGE);


#endif // WI_SHADERINTEROP_IMAGE_H
