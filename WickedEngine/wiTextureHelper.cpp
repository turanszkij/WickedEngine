#include "wiTextureHelper.h"
#include "wiRenderer.h"
#include "wiRandom.h"
#include "wiColor.h"
#include "wiBackLog.h"
#include "wiSpinLock.h"

#include <unordered_map>

using namespace wiGraphics;

// from Utility/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp
extern float samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension);

namespace wiTextureHelper
{

	enum HELPERTEXTURES
	{
		HELPERTEXTURE_RANDOM64X64,
		HELPERTEXTURE_COLORGRADEDEFAULT,
		HELPERTEXTURE_NORMALMAPDEFAULT,
		HELPERTEXTURE_BLACKCUBEMAP,
		HELPERTEXTURE_UINT4,
		HELPERTEXTURE_BLUENOISE,
		HELPERTEXTURE_COUNT
	};
	wiGraphics::Texture helperTextures[HELPERTEXTURE_COUNT];
	std::unordered_map<unsigned long, wiGraphics::Texture> colorTextures;
	wiSpinLock colorlock;

	void Initialize()
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		// Random64x64
		{
			uint8_t data[64 * 64 * 4];
			for (int i = 0; i < arraysize(data); i += 4)
			{
				data[i] = wiRandom::getRandom(0, 255);
				data[i + 1] = wiRandom::getRandom(0, 255);
				data[i + 2] = wiRandom::getRandom(0, 255);
				data[i + 3] = wiRandom::getRandom(0, 255);
			}

			CreateTexture(helperTextures[HELPERTEXTURE_RANDOM64X64], data, 64, 64);
			device->SetName(&helperTextures[HELPERTEXTURE_RANDOM64X64], "HELPERTEXTURE_RANDOM64X64");
		}

		// ColorGradeDefault
		{
			uint8_t data[256 * 16 * 4];
			for (uint8_t slice = 0; slice < 16; ++slice)
			{
				for (int x = 0; x < 16; ++x)
				{
					for (int y = 0; y < 16; ++y)
					{
						uint8_t r = x * 16 + x;
						uint8_t g = y * 16 + y;
						uint8_t b = slice * 16 + slice;

						int gridPos = (slice * 16 + y * 256 + x) * 4;
						data[gridPos] = r;
						data[gridPos + 1] = g;
						data[gridPos + 2] = b;
						data[gridPos + 3] = 255;
					}
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], data, 256, 16);
			device->SetName(&helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], "HELPERTEXTURE_COLORGRADEDEFAULT");
		}

		// BlackCubemap
		{
			const int width = 1;
			const int height = 1;

			struct vector4b
			{
				unsigned char r;
				unsigned char g;
				unsigned char b;
				unsigned char a;

				vector4b(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 0) :r(r), g(g), b(b), a(a) {}
			};

			TextureDesc texDesc;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 6;
			texDesc.Format = FORMAT_R8G8B8A8_UNORM;
			texDesc.CPUAccessFlags = 0;
			texDesc.SampleCount = 1;
			texDesc.Usage = USAGE_DEFAULT;
			texDesc.BindFlags = BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

			SubresourceData pData[6];
			vector4b d[6][width * height]; // 6 images of type vector4b = 4 * unsigned char

			for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
			{
				// fill with black color  
				for (int pix = 0; pix < width*height; ++pix)
				{
					d[cubeMapFaceIndex][pix] = vector4b(0, 0, 0, 0);
				}

				pData[cubeMapFaceIndex].pSysMem = &d[cubeMapFaceIndex][0];// description.data;
				pData[cubeMapFaceIndex].SysMemPitch = width * 4;
				pData[cubeMapFaceIndex].SysMemSlicePitch = 0;
			}

			device->CreateTexture(&texDesc, &pData[0], &helperTextures[HELPERTEXTURE_BLACKCUBEMAP]);
			device->SetName(&helperTextures[HELPERTEXTURE_BLACKCUBEMAP], "HELPERTEXTURE_BLACKCUBEMAP");
		}

		// UINT4:
		{
			uint8_t data[16] = {};
			CreateTexture(helperTextures[HELPERTEXTURE_UINT4], data, 1, 1, FORMAT_R32G32B32A32_UINT);
			device->SetName(&helperTextures[HELPERTEXTURE_UINT4], "HELPERTEXTURE_UINT4");
		}

		// Blue Noise:
		{
			uint8_t blueNoise[128][128][4] = {};

			for (int x = 0; x < 128; ++x)
			{
				for (int y = 0; y < 128; ++y)
				{
					float const f0 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 0);
					float const f1 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 1);
					float const f2 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 2);
					float const f3 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 3);

					blueNoise[x][y][0] = static_cast<uint8_t>(f0 * 0xFF);
					blueNoise[x][y][1] = static_cast<uint8_t>(f1 * 0xFF);
					blueNoise[x][y][2] = static_cast<uint8_t>(f2 * 0xFF);
					blueNoise[x][y][3] = static_cast<uint8_t>(f3 * 0xFF);
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_BLUENOISE], (uint8_t*)blueNoise, 128, 128, FORMAT_R8G8B8A8_UNORM);
			device->SetName(&helperTextures[HELPERTEXTURE_BLUENOISE], "HELPERTEXTURE_BLUENOISE");
		}

		wiBackLog::post("wiTextureHelper Initialized");
	}

	const Texture* getRandom64x64()
	{
		return &helperTextures[HELPERTEXTURE_RANDOM64X64];
	}

	const Texture* getColorGradeDefault()
	{
		return &helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT];
	}

	const Texture* getNormalMapDefault()
	{
		return getColor(wiColor(127, 127, 255, 255));
	}

	const Texture* getBlackCubeMap()
	{
		return &helperTextures[HELPERTEXTURE_BLACKCUBEMAP];
	}

	const Texture* getUINT4()
	{
		return &helperTextures[HELPERTEXTURE_UINT4];
	}

	const Texture* getBlueNoise()
	{
		return &helperTextures[HELPERTEXTURE_BLUENOISE];
	}

	const Texture* getWhite()
	{
		return getColor(wiColor(255, 255, 255, 255));
	}

	const Texture* getBlack()
	{
		return getColor(wiColor(0, 0, 0, 255));
	}

	const Texture* getTransparent()
	{
		return getColor(wiColor(0, 0, 0, 0));
	}

	const Texture* getColor(wiColor color)
	{
		colorlock.lock();
		auto it = colorTextures.find(color.rgba);
		auto end = colorTextures.end();
		colorlock.unlock();

		if (it != end)
		{
			return &it->second;
		}

		GraphicsDevice* device = wiRenderer::GetDevice();

		static const int dim = 1;
		static const int dataLength = dim * dim * 4;
		uint8_t data[dataLength];
		for (int i = 0; i < dataLength; i += 4)
		{
			data[i] = color.getR();
			data[i + 1] = color.getG();
			data[i + 2] = color.getB();
			data[i + 3] = color.getA();
		}

		Texture texture;
		if (CreateTexture(texture, data, dim, dim) == false)
		{
			return nullptr;
		}
		device->SetName(&texture, "HELPERTEXTURE_COLOR");

		colorlock.lock();
		colorTextures[color.rgba] = texture;
		colorlock.unlock();

		return &colorTextures[color.rgba];
	}


	bool CreateTexture(wiGraphics::Texture& texture, const uint8_t* data, uint32_t width, uint32_t height, FORMAT format)
	{
		if (data == nullptr)
		{
			return false;
		}
		GraphicsDevice* device = wiRenderer::GetDevice();

		TextureDesc textureDesc;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleCount = 1;
		textureDesc.Usage = USAGE_IMMUTABLE;
		textureDesc.BindFlags = BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		SubresourceData InitData;
		InitData.pSysMem = data;
		InitData.SysMemPitch = width * device->GetFormatStride(format);

		return device->CreateTexture(&textureDesc, &InitData, &texture);
	}

}
