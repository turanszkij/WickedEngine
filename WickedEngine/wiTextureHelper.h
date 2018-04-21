#pragma once
#include "wiRenderer.h"
#include "wiColor.h"
#include "CommonInclude.h"


class wiTextureHelper
{
private:
	class wiTextureHelperInstance
	{
	private:
		enum HELPERTEXTURES
		{
			HELPERTEXTURE_RANDOM64X64,
			HELPERTEXTURE_COLORGRADEDEFAULT,
			HELPERTEXTURE_NORMALMAPDEFAULT,
			HELPERTEXTURE_BLACKCUBEMAP,
			HELPERTEXTURE_COUNT
		};
		wiGraphicsTypes::Texture2D* helperTextures[HELPERTEXTURE_COUNT];

		std::unordered_map<unsigned long, wiGraphicsTypes::Texture2D*> colorTextures;
	public:
		wiTextureHelperInstance();
		~wiTextureHelperInstance();

		wiGraphicsTypes::Texture2D* getRandom64x64();
		wiGraphicsTypes::Texture2D* getColorGradeDefault();
		wiGraphicsTypes::Texture2D* getNormalMapDefault();
		wiGraphicsTypes::Texture2D* getBlackCubeMap();

		wiGraphicsTypes::Texture2D* getWhite();
		wiGraphicsTypes::Texture2D* getBlack();
		wiGraphicsTypes::Texture2D* getTransparent();
		wiGraphicsTypes::Texture2D* getColor(const wiColor& color);
	};


public:
	static wiTextureHelperInstance* getInstance()
	{
		static wiTextureHelperInstance* instance = new wiTextureHelperInstance();
		return instance;
	}

	template<typename T>
	static HRESULT CreateTexture(wiGraphicsTypes::Texture2D*& texture, T* data, UINT width, UINT height, UINT channelCount, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM)
	{
		if (data == nullptr)
		{
			return E_FAIL;
		}
		using namespace wiGraphicsTypes;

		TextureDesc textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = USAGE_IMMUTABLE;
		textureDesc.BindFlags = BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		SubresourceData InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = data;
		InitData.SysMemPitch = static_cast<UINT>(width * channelCount);

		HRESULT hr;
		hr = wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, &InitData, &texture);

		return hr;
	}
};

