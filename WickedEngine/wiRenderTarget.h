#pragma once
#include "CommonInclude.h"

class wiDepthTarget;

class wiRenderTarget
{
private:
	int numViews;
	bool retargetted;
	vector<ID3D11ShaderResourceView*>	SAVEDshaderResource;
	void clear();
	D3D11_TEXTURE2D_DESC				textureDesc;
public:
	D3D11_VIEWPORT						viewPort;
	vector<ID3D11Texture2D*>			texture2D;
	vector<ID3D11RenderTargetView*>		renderTarget;
	vector<ID3D11ShaderResourceView*>	shaderResource;
	wiDepthTarget*						depth;

	wiRenderTarget();
	wiRenderTarget(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	~wiRenderTarget();

	void Initialize(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	void InitializeCube(UINT size, int numViews, bool hasDepth, DXGI_FORMAT format);
	void InitializeCube(UINT size, int numViews, bool hasDepth);
	void Activate(ID3D11DeviceContext*);
	void Activate(ID3D11DeviceContext* context, float r, float g, float b, float a);
	void Activate(ID3D11DeviceContext* context, wiDepthTarget*, float r, float g, float b, float a);
	void Activate(ID3D11DeviceContext* context, wiDepthTarget*);
	void Set(ID3D11DeviceContext* context);
	void Set(ID3D11DeviceContext* context, wiDepthTarget*);
	void Retarget(ID3D11ShaderResourceView* resource);
	void Restore();

	D3D11_TEXTURE2D_DESC GetDesc() const{ return textureDesc; }
	UINT GetMipCount();
};

