#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiDepthTarget;

class wiRenderTarget
{
private:
	int numViews;
	bool retargetted;
	vector<TextureView>	SAVEDshaderResource;
	void clear();
	Texture2DDesc				textureDesc;
public:
	ViewPort						viewPort;
	vector<Texture2D>			texture2D;
	vector<RenderTargetView>		renderTarget;
	vector<TextureView>	shaderResource;
	wiDepthTarget*						depth;

	wiRenderTarget();
	wiRenderTarget(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	~wiRenderTarget();

	void Initialize(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	void InitializeCube(UINT size, int numViews, bool hasDepth, DXGI_FORMAT format, UINT mipMapLevelCount = 1);
	void InitializeCube(UINT size, int numViews, bool hasDepth);
	void Activate(DeviceContext);
	void Activate(DeviceContext context, float r, float g, float b, float a);
	void Activate(DeviceContext context, wiDepthTarget*, float r, float g, float b, float a);
	void Activate(DeviceContext context, wiDepthTarget*);
	void Deactivate(DeviceContext context);
	void Set(DeviceContext context);
	void Set(DeviceContext context, wiDepthTarget*);
	void Retarget(TextureView resource);
	void Restore();

	Texture2DDesc GetDesc() const{ return textureDesc; }
	UINT GetMipCount();
};

