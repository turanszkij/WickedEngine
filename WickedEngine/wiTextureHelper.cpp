#include "wiTextureHelper.h"
#include "wiRandom.h"

using namespace wiGraphicsTypes;

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
		SAFE_DELETE(helperTextures[i]);
	}

	for (auto& x : colorTextures)
	{
		SAFE_DELETE(x.second);
	}
}



Texture2D* wiTextureHelper::wiTextureHelperInstance::getRandom64x64()
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

Texture2D* wiTextureHelper::wiTextureHelperInstance::getColorGradeDefault()
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

Texture2D* wiTextureHelper::wiTextureHelperInstance::getNormalMapDefault()
{
	return getColor(wiColor(127, 127, 255, 255));
}

Texture2D* wiTextureHelper::wiTextureHelperInstance::getBlackCubeMap()
{
	if (helperTextures[HELPERTEXTURE_BLACKCUBEMAP] != nullptr)
	{
		return helperTextures[HELPERTEXTURE_BLACKCUBEMAP];
	}

	int width = 1;
	int height = 1;

	struct vector4b
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;

		vector4b(unsigned char r=0, unsigned char g=0, unsigned char b=0, unsigned char a=0) :r(r), g(g), b(b), a(a) {}
	};

	TextureDesc texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = FORMAT_R8G8B8A8_UNORM;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = USAGE_DEFAULT;
	texDesc.BindFlags = BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	SubresourceData pData[6];
	std::vector<vector4b> d[6]; // 6 images of type vector4b = 4 * unsigned char

	for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
	{
		d[cubeMapFaceIndex].resize(width * height);

		// fill with black color  
		std::fill(
			d[cubeMapFaceIndex].begin(),
			d[cubeMapFaceIndex].end(),
			vector4b(0, 0, 0, 0));

		pData[cubeMapFaceIndex].pSysMem = &d[cubeMapFaceIndex][0];// description.data;
		pData[cubeMapFaceIndex].SysMemPitch = width * 4;
		pData[cubeMapFaceIndex].SysMemSlicePitch = 0;
	}

	HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&texDesc, &pData[0], &helperTextures[HELPERTEXTURE_BLACKCUBEMAP]);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return helperTextures[HELPERTEXTURE_BLACKCUBEMAP];
}

Texture2D* wiTextureHelper::wiTextureHelperInstance::getWhite()
{
	return getColor(wiColor(255, 255, 255, 255));
}

Texture2D* wiTextureHelper::wiTextureHelperInstance::getBlack()
{
	return getColor(wiColor(0, 0, 0, 255));
}

Texture2D* wiTextureHelper::wiTextureHelperInstance::getTransparent()
{
	return getColor(wiColor(0, 0, 0, 0));
}

Texture2D* wiTextureHelper::wiTextureHelperInstance::getColor(const wiColor& color)
{
	if (colorTextures.find(color.rgba) != colorTextures.end())
	{
		return colorTextures[color.rgba];
	}

	static const int dim = 1;
	static const int dataLength = dim * dim * 4;
	unsigned char* data = new unsigned char[dataLength];
	for (int i = 0; i < dataLength; i += 4)
	{
		data[i] = color.r;
		data[i + 1] = color.g;
		data[i + 2] = color.b;
		data[i + 3] = color.a;
	}

	Texture2D* texture = nullptr;
	if (FAILED(CreateTexture(texture, data, dim, dim, 4)))
	{
		delete[] data;
		return nullptr;
	}
	delete[] data;

	colorTextures[color.rgba] = texture;

	return texture;
}

