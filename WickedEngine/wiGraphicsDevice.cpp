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

uint32_t GraphicsDevice::GetFormatStride(FORMAT value)
{
	switch (value)
	{
	case FORMAT_R32G32B32A32_FLOAT:
		return 16;
		break;
	case FORMAT_R32G32_FLOAT:
		return 8;
		break;
	case FORMAT_R16G16_FLOAT:
	case FORMAT_R32_FLOAT:
	case FORMAT_R32_UINT:
	case FORMAT_R8G8B8A8_UINT:
	case FORMAT_R8G8B8A8_SINT:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_R8G8B8A8_SNORM:
		return 4;
		break;
	case FORMAT_R16_FLOAT:
		return 2;
		break;
	}

	// TODO more formats

	return 16;
}

