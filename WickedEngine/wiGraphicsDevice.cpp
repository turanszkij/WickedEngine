#include "wiGraphicsDevice.h"

using namespace wiGraphics;

bool GraphicsDevice::CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const
{
	switch (capability)
	{
	case wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION:
		return TESSELLATION;
		break;
	case wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING:
		return MULTITHREADED_RENDERING;
		break;
	case wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION:
		return CONSERVATIVE_RASTERIZATION;
		break;
	case wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS:
		return RASTERIZER_ORDERED_VIEWS;
		break;
	case wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT:
		return UNORDEREDACCESSTEXTURE_LOAD_EXT;
		break;
	default:
		break;
	}
	return false;
}

uint32_t GraphicsDevice::GetFormatStride(FORMAT value) const
{
	switch (value)
	{
	case FORMAT_R32G32B32A32_FLOAT:
		return 16;
	case FORMAT_R32G32_FLOAT:
	case FORMAT_R32G32_UINT:
	case FORMAT_R32G32_SINT:
	case FORMAT_R16G16B16A16_FLOAT:
		return 8;
	case FORMAT_R11G11B10_FLOAT:
	case FORMAT_R16G16_FLOAT:
	case FORMAT_R16G16_UINT:
	case FORMAT_R16G16_SINT:
	case FORMAT_R32_FLOAT:
	case FORMAT_R32_UINT:
	case FORMAT_R8G8B8A8_UINT:
	case FORMAT_R8G8B8A8_SINT:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_R8G8B8A8_SNORM:
	case FORMAT_R10G10B10A2_UNORM:
		return 4;
	case FORMAT_R16_FLOAT:
	case FORMAT_R16_UINT:
	case FORMAT_R16_SINT:
	case FORMAT_R16_UNORM:
	case FORMAT_R16_SNORM:
		return 2;
	case FORMAT_R8_TYPELESS:
	case FORMAT_R8_UNORM:
	case FORMAT_R8_SNORM:
	case FORMAT_R8_UINT:
	case FORMAT_R8_SINT:
	case FORMAT_A8_UNORM:
		return 1;
	default:
		assert(0);
		break;
	}

	// Probably didn't catch all...

	return 16;
}

bool GraphicsDevice::IsFormatUnorm(FORMAT value) const
{
	switch (value)
	{
	case FORMAT_B8G8R8A8_UNORM:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_R10G10B10A2_UNORM:
	case FORMAT_R16_UNORM:
	case FORMAT_R8_UNORM:
	case FORMAT_A8_UNORM:
		return true;
	}

	// Probably didn't catch all...

	return false;
}

