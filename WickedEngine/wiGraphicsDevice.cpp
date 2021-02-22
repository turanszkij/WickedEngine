#include "wiGraphicsDevice.h"
#include "wiEvent.h"

using namespace wiGraphics;

uint32_t GraphicsDevice::GetFormatStride(FORMAT value) const
{
	switch (value)
	{
		
	case FORMAT_R32G32B32A32_FLOAT:
	case FORMAT_R32G32B32A32_UINT:
	case FORMAT_R32G32B32A32_SINT:
	case FORMAT_BC1_UNORM:
	case FORMAT_BC1_UNORM_SRGB:
	case FORMAT_BC2_UNORM:
	case FORMAT_BC2_UNORM_SRGB:
	case FORMAT_BC3_UNORM:
	case FORMAT_BC3_UNORM_SRGB:
	case FORMAT_BC4_SNORM:
	case FORMAT_BC4_UNORM:
	case FORMAT_BC5_SNORM:
	case FORMAT_BC5_UNORM:
	case FORMAT_BC6H_UF16:
	case FORMAT_BC6H_SF16:
	case FORMAT_BC7_UNORM:
	case FORMAT_BC7_UNORM_SRGB:
		return 16;

	case FORMAT_R32G32B32_FLOAT:
	case FORMAT_R32G32B32_UINT:
	case FORMAT_R32G32B32_SINT:
		return 12;

	case FORMAT_R16G16B16A16_FLOAT:
	case FORMAT_R16G16B16A16_UNORM:
	case FORMAT_R16G16B16A16_UINT:
	case FORMAT_R16G16B16A16_SNORM:
	case FORMAT_R16G16B16A16_SINT:
		return 8;

	case FORMAT_R32G32_FLOAT:
	case FORMAT_R32G32_UINT:
	case FORMAT_R32G32_SINT:
	case FORMAT_R32G8X24_TYPELESS:
	case FORMAT_D32_FLOAT_S8X24_UINT:
		return 8;

	case FORMAT_R10G10B10A2_UNORM:
	case FORMAT_R10G10B10A2_UINT:
	case FORMAT_R11G11B10_FLOAT:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_R8G8B8A8_UNORM_SRGB:
	case FORMAT_R8G8B8A8_UINT:
	case FORMAT_R8G8B8A8_SNORM:
	case FORMAT_R8G8B8A8_SINT:
	case FORMAT_B8G8R8A8_UNORM:
	case FORMAT_B8G8R8A8_UNORM_SRGB:
	case FORMAT_R16G16_FLOAT:
	case FORMAT_R16G16_UNORM:
	case FORMAT_R16G16_UINT:
	case FORMAT_R16G16_SNORM:
	case FORMAT_R16G16_SINT:
	case FORMAT_R32_TYPELESS:
	case FORMAT_D32_FLOAT:
	case FORMAT_R32_FLOAT:
	case FORMAT_R32_UINT:
	case FORMAT_R32_SINT:
	case FORMAT_R24G8_TYPELESS:
	case FORMAT_D24_UNORM_S8_UINT:
		return 4;

	case FORMAT_R8G8_UNORM:
	case FORMAT_R8G8_UINT:
	case FORMAT_R8G8_SNORM:
	case FORMAT_R8G8_SINT:
	case FORMAT_R16_TYPELESS:
	case FORMAT_R16_FLOAT:
	case FORMAT_D16_UNORM:
	case FORMAT_R16_UNORM:
	case FORMAT_R16_UINT:
	case FORMAT_R16_SNORM:
	case FORMAT_R16_SINT:
		return 2;

	case FORMAT_R8_UNORM:
	case FORMAT_R8_UINT:
	case FORMAT_R8_SNORM:
	case FORMAT_R8_SINT:
		return 1;


	default:
		assert(0); // didn't catch format!
		break;
	}

	return 16;
}

bool GraphicsDevice::IsFormatUnorm(FORMAT value) const
{
	switch (value)
	{
	case FORMAT_R16G16B16A16_UNORM:
	case FORMAT_R10G10B10A2_UNORM:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_R8G8B8A8_UNORM_SRGB:
	case FORMAT_B8G8R8A8_UNORM:
	case FORMAT_B8G8R8A8_UNORM_SRGB:
	case FORMAT_R16G16_UNORM:
	case FORMAT_D24_UNORM_S8_UINT:
	case FORMAT_R8G8_UNORM:
	case FORMAT_D16_UNORM:
	case FORMAT_R16_UNORM:
	case FORMAT_R8_UNORM:
		return true;
	}

	return false;
}

bool GraphicsDevice::IsFormatBlockCompressed(FORMAT value) const
{
	switch (value)
	{
	case FORMAT_BC1_UNORM:
	case FORMAT_BC1_UNORM_SRGB:
	case FORMAT_BC2_UNORM:
	case FORMAT_BC2_UNORM_SRGB:
	case FORMAT_BC3_UNORM:
	case FORMAT_BC3_UNORM_SRGB:
	case FORMAT_BC4_UNORM:
	case FORMAT_BC4_SNORM:
	case FORMAT_BC5_UNORM:
	case FORMAT_BC5_SNORM:
	case FORMAT_BC6H_UF16:
	case FORMAT_BC6H_SF16:
	case FORMAT_BC7_UNORM:
	case FORMAT_BC7_UNORM_SRGB:
		return true;
	}

	return false;
}

bool GraphicsDevice::IsFormatStencilSupport(FORMAT value) const
{
	switch (value)
	{
	case FORMAT_R32G8X24_TYPELESS:
	case FORMAT_D32_FLOAT_S8X24_UINT:
	case FORMAT_R24G8_TYPELESS:
	case FORMAT_D24_UNORM_S8_UINT:
		return true;
	}

	return false;
}

float GraphicsDevice::GetScreenWidth() const
{
	return (float)GetResolutionWidth() / GetDPIScaling();
}
float GraphicsDevice::GetScreenHeight() const
{
	return (float)GetResolutionHeight() / GetDPIScaling();
}
