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
			HELPERTEXTURE_COUNT
		};
		Texture2D* helperTextures[HELPERTEXTURE_COUNT];

		unordered_map<unsigned long, Texture2D*> colorTextures;
	public:
		wiTextureHelperInstance();
		~wiTextureHelperInstance();

		Texture2D* getRandom64x64();
		Texture2D* getColorGradeDefault();
		Texture2D* getNormalMapDefault();

		Texture2D* getWhite();
		Texture2D* getBlack();
		Texture2D* getTransparent();
		Texture2D* getColor(const wiColor& color);
	};

	static wiTextureHelperInstance* instance;

public:
	inline static wiTextureHelperInstance* getInstance()
	{
		if (instance != nullptr)
			return instance;

		return instance = new wiTextureHelperInstance();
	}

	template<typename T>
	static HRESULT CreateTexture(Texture2D*& texture, T* data, UINT width, UINT height, UINT channelCount, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		if (data == nullptr)
		{
			return E_FAIL;
		}

		Texture2DDesc textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = USAGE_DEFAULT;
		textureDesc.BindFlags = BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		//ShaderResourceViewDesc shaderResourceViewDesc;
		//shaderResourceViewDesc.Format = format;
		//shaderResourceViewDesc.ViewDimension = RESOURCE_DIMENSION_TEXTURE2D;
		////shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		////shaderResourceViewDesc.Texture2D.MipLevels = 1;
		//shaderResourceViewDesc.mipLevels = 1;

		SubresourceData InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = data;
		InitData.SysMemPitch = static_cast<UINT>(width * channelCount);

		//Texture2D* texture2D = nullptr;
		HRESULT hr;
		hr = wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, &InitData, &texture);
		//if (FAILED(hr))
		//{
		//	return hr;
		//}
		//hr = wiRenderer::GetDevice()->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &texture);
		//SAFE_RELEASE(texture2D);

		return hr;
	}
};

