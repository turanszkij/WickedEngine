#ifndef WI_SHADERINTEROP_PAINT_H
#define WI_SHADERINTEROP_PAINT_H
#include "ShaderInterop.h"

static const uint PAINT_TEXTURE_BLOCKSIZE = 8;

CBUFFER(PaintTextureCB, CBSLOT_RENDERER_UTILITY)
{
	uint2 xPaintBrushCenter;
	uint xPaintBrushRadius;
	float xPaintBrushAmount;

	float xPaintBrushFalloff;
	uint xPaintBrushColor;
	uint2 padding0;
};

CBUFFER(PaintRadiusCB, CBSLOT_RENDERER_UTILITY)
{
	uint2 xPaintRadResolution;
	uint2 xPaintRadCenter;
	uint xPaintRadUVSET;
	float xPaintRadRadius;
	uint2 pad;
};

#endif // WI_SHADERINTEROP_PAINT_H
