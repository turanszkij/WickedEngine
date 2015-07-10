#include "wiTextureHelper.h"
#include "wiRandom.h"

wiTextureHelper::wiTextureHelperInstance* wiTextureHelper::instance = nullptr;

wiTextureHelper::wiTextureHelperInstance::wiTextureHelperInstance()
{
	randomTexture = nullptr;
	whiteTexture = nullptr;
	blackTexture = nullptr;
	colorGradeDefaultTexture = nullptr;
}
wiTextureHelper::wiTextureHelperInstance::~wiTextureHelperInstance()
{
	wiRenderer::SafeRelease(randomTexture);
	wiRenderer::SafeRelease(whiteTexture);
	wiRenderer::SafeRelease(blackTexture);
	wiRenderer::SafeRelease(colorGradeDefaultTexture);

	for (auto& x : colorTextures)
	{
		wiRenderer::SafeRelease(x.second);
	}
}



wiRenderer::TextureView wiTextureHelper::wiTextureHelperInstance::getRandom64x64()
{
	if (randomTexture != nullptr)
	{
		return randomTexture;
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

	if (FAILED(CreateTexture(randomTexture, data, 64, 64, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return randomTexture;
}

wiRenderer::TextureView wiTextureHelper::wiTextureHelperInstance::getColorGradeDefaultTex()
{
	if (colorGradeDefaultTexture != nullptr)
	{
		return colorGradeDefaultTexture;
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

	if (FAILED(CreateTexture(colorGradeDefaultTexture, data, 256, 16, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return colorGradeDefaultTexture;
}

wiRenderer::TextureView wiTextureHelper::wiTextureHelperInstance::getWhite()
{
	if (whiteTexture != nullptr)
	{
		return whiteTexture;
	}

	static const int dataLength = 2 * 2 * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int i = 0; i < dataLength; ++i)
	{
		data[i] = 255;
	}

	if (FAILED(CreateTexture(whiteTexture, data, 2, 2, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return whiteTexture;
}

wiRenderer::TextureView wiTextureHelper::wiTextureHelperInstance::getBlack()
{
	if (blackTexture != nullptr)
	{
		return blackTexture;
	}

	static const int dataLength = 2 * 2 * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int i = 0; i < dataLength; i += 4)
	{
		data[i] = 0;
		data[i + 1] = 0;
		data[i + 2] = 0;
		data[i + 3] = 255;
	}

	if (FAILED(CreateTexture(blackTexture, data, 2, 2, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;


	return blackTexture;
}

wiRenderer::TextureView wiTextureHelper::wiTextureHelperInstance::getColor(const wiColor& color)
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

	wiRenderer::TextureView texture;
	if (FAILED(CreateTexture(texture, data, 2, 2, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;

	colorTextures[color.rgba] = texture;

	return texture;
}

