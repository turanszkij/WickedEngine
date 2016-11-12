#include "wiGraphicsDevice.h"

using namespace wiGraphicsTypes;

//FORMAT GraphicsDevice::BACKBUFFER_FORMAT = FORMAT::FORMAT_R8G8B8A8_UNORM;
FORMAT GraphicsDevice::BACKBUFFER_FORMAT = FORMAT::FORMAT_R10G10B10A2_UNORM;

FORMAT GraphicsDevice::GetBackBufferFormat()
{
	return BACKBUFFER_FORMAT;
}
