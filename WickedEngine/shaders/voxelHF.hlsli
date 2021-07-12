#ifndef WI_VOXEL_HF
#define WI_VOXEL_HF

struct VoxelType
{
	uint colorMask;
	uint normalMask;
};

static const float __hdrRange = 10.0f;

// Encode HDR color to a 32 bit uint
// Alpha is 1 bit + 7 bit HDR remapping
uint PackVoxelColor(in float4 color)
{
	// normalize color to LDR
	float hdr = length(color.rgb);
	color.rgb /= hdr;

	// encode LDR color and HDR range
	uint3 iColor = uint3(color.rgb * 255.0f);
	uint iHDR = (uint)(saturate(hdr / __hdrRange) * 127);
	uint colorMask = (iHDR << 24u) | (iColor.r << 16u) | (iColor.g << 8u) | iColor.b;

	// encode alpha into highest bit
	uint iAlpha = (color.a > 0 ? 1u : 0u);
	colorMask |= iAlpha << 31u;

	return colorMask;
}

// Decode 32 bit uint into HDR color with 1 bit alpha
float4 UnpackVoxelColor(in uint colorMask)
{
	float hdr;
	float4 color;

	hdr = (colorMask >> 24u) & 0x0000007f;
	color.r = (colorMask >> 16u) & 0x000000ff;
	color.g = (colorMask >> 8u) & 0x000000ff;
	color.b = colorMask & 0x000000ff;

	hdr /= 127.0f;
	color.rgb /= 255.0f;

	color.rgb *= hdr * __hdrRange;

	color.a = (colorMask >> 31u) & 0x00000001;

	return color;
}

#endif // WI_VOXEL_HF
