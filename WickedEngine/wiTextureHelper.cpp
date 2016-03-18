#include "wiTextureHelper.h"
#include "wiRandom.h"

wiTextureHelper::wiTextureHelperInstance* wiTextureHelper::instance = nullptr;

wiTextureHelper::wiTextureHelperInstance::wiTextureHelperInstance()
{
	for (int i = 0; i < HELPERTEXTURE_COUNT; ++i)
	{
		helperTextures[i] = nullptr;
	}
}
wiTextureHelper::wiTextureHelperInstance::~wiTextureHelperInstance()
{
	for (int i = 0; i < HELPERTEXTURE_COUNT; ++i)
	{
		SAFE_RELEASE(helperTextures[i]);
	}

	for (auto& x : colorTextures)
	{
		SAFE_RELEASE(x.second);
	}
}



TextureView wiTextureHelper::wiTextureHelperInstance::getRandom64x64()
{
	if (helperTextures[HELPERTEXTURE_RANDOM64X64] != nullptr)
	{
		return helperTextures[HELPERTEXTURE_RANDOM64X64];
	}

	static const int dataLength = 64 * 64 * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int i = 0; i < dataLength; i += 4)
	{
		data[i] = wiRandom::getRandom(0, 255);
		data[i + 1] = wiRandom::getRandom(0, 255);
		data[i + 2] = wiRandom::getRandom(0, 255);
		data[i + 3] = 255;
	}

	if (FAILED(CreateTexture(helperTextures[HELPERTEXTURE_RANDOM64X64], data, 64, 64, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return helperTextures[HELPERTEXTURE_RANDOM64X64];
}

TextureView wiTextureHelper::wiTextureHelperInstance::getColorGradeDefault()
{
	if (helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT] != nullptr)
	{
		return helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT];
	}

	static const int dataLength = 256 * 16 * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int slice = 0; slice < 16; ++slice)
	{
		for (int x = 0; x < 16; ++x)
		{
			for (int y = 0; y < 16; ++y)
			{
				wiColor color;
				color.r = x * 16 + x;
				color.g = y * 16 + y;
				color.b = slice * 16 + slice;

				int gridPos = (slice * 16 + y * 256 + x) * 4;
				data[gridPos] = color.r;
				data[gridPos + 1] = color.g;
				data[gridPos + 2] = color.b;
				data[gridPos + 3] = color.a;
			}
		}
	}

	if (FAILED(CreateTexture(helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], data, 256, 16, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT];
}

TextureView wiTextureHelper::wiTextureHelperInstance::getNormalMapDefault()
{
	return getColor(wiColor(127, 127, 255, 255));
}

TextureView wiTextureHelper::wiTextureHelperInstance::getWhite()
{
	return getColor(wiColor(255, 255, 255, 255));
}

TextureView wiTextureHelper::wiTextureHelperInstance::getBlack()
{
	return getColor(wiColor(0, 0, 0, 255));
}

TextureView wiTextureHelper::wiTextureHelperInstance::getTransparent()
{
	return getColor(wiColor(0, 0, 0, 0));
}

TextureView wiTextureHelper::wiTextureHelperInstance::getColor(const wiColor& color)
{
	if (colorTextures.find(color.rgba) != colorTextures.end())
	{
		return colorTextures[color.rgba];
	}

	static const int dataLength = 2 * 2 * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int i = 0; i < dataLength; i += 4)
	{
		data[i] = color.r;
		data[i + 1] = color.g;
		data[i + 2] = color.b;
		data[i + 3] = color.a;
	}

	TextureView texture;
	if (FAILED(CreateTexture(texture, data, 2, 2, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;

	colorTextures[color.rgba] = texture;

	return texture;
}

