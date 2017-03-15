#include "wiGraphicsDevice.h"

using namespace wiGraphicsTypes;

//FORMAT GraphicsDevice::BACKBUFFER_FORMAT = FORMAT::FORMAT_R8G8B8A8_UNORM;
FORMAT GraphicsDevice::BACKBUFFER_FORMAT = FORMAT::FORMAT_R10G10B10A2_UNORM;

FORMAT GraphicsDevice::GetBackBufferFormat()
{
	return BACKBUFFER_FORMAT;
}

bool GraphicsDevice::CheckCapability(GRAPHICSDEVICE_CAPABILITY capability)
{
	switch (capability)
	{
	case wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION:
		return TESSELLATION;
		break;
	case wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING:
		return MULTITHREADED_RENDERING;
		break;
	case wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION:
		return CONSERVATIVE_RASTERIZATION;
		break;
	case wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS:
		return RASTERIZER_ORDERED_VIEWS;
		break;
	case wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT:
		return UNORDEREDACCESSTEXTURE_LOAD_EXT;
		break;
	default:
		break;
	}
	return false;
}
