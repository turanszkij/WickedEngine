#pragma once
#include "Renderer.h"
#include "wiColor.h"
#include "CommonInclude.h"


static class wiTextureHelper
{
private:
	class wiTextureHelperInstance
	{
	private:
		wiRenderer::TextureView randomTexture;
		wiRenderer::TextureView colorGradeDefaultTexture;
		wiRenderer::TextureView whiteTexture;
		wiRenderer::TextureView blackTexture;

		unordered_map<unsigned long, wiRenderer::TextureView> colorTextures;
	public:
		wiTextureHelperInstance();
		~wiTextureHelperInstance();

		wiRenderer::TextureView getRandom64x64();
		wiRenderer::TextureView getColorGradeDefaultTex();

		wiRenderer::TextureView getWhite();
		wiRenderer::TextureView getBlack();
		wiRenderer::TextureView getColor(const wiColor& color);
	};

	static wiTextureHelperInstance* instance;

public:
	__forceinline static wiTextureHelperInstance* getInstance()
	{
		if (instance != nullptr)
			return instance;

		return instance = new wiTextureHelperInstance();
	}

	template<class T>
	static HRESULT CreateTexture(wiRenderer::TextureView& texture, T* data, UINT width, UINT height, UINT channelCount, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		if (data == nullptr)
		{
			return E_FAIL;
		}

		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.Format = format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = data;
		InitData.SysMemPitch = static_cast<UINT>(width * channelCount);

		ID3D11Texture2D* texture2D = nullptr;
		HRESULT hr;
		hr = wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, &InitData, &texture2D);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &texture);
		wiRenderer::SafeRelease(texture2D);

		return hr;
	}
};

