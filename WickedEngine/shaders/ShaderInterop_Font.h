#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H
#include "ShaderInterop.h"

struct PushConstantsFont
{
	float4x4 transform;
	uint color;
	int buffer_index;
	uint buffer_offset;
	int texture_index;
};
PUSHCONSTANT(push, PushConstantsFont);

#undef WICKED_ENGINE_ROOTSIGNATURE_GRAPHICS
#define WICKED_ENGINE_ROOTSIGNATURE_GRAPHICS \
	"RootConstants(num32BitConstants=32, b999), " \
	"DescriptorTable( " \
		"SRV(t0, space = 2, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 3, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	"), " \
	"StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"


#endif // WI_SHADERINTEROP_FONT_H
