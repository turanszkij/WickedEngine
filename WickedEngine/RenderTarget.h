#pragma once
#include "WickedEngine.h"
#include "DepthTarget.h"

class DepthTarget;

class RenderTarget
{
private:
	int numViews;
	bool retargetted;
	vector<ID3D11ShaderResourceView*>	SAVEDshaderResource;
	void clear();
public:
	D3D11_VIEWPORT						viewPort;
	vector<ID3D11Texture2D*>			texture2D;
	vector<ID3D11RenderTargetView*>		renderTarget;
	vector<ID3D11ShaderResourceView*>	shaderResource;
	DepthTarget*						depth;

	RenderTarget();
	RenderTarget(int width, int height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ, DXGI_FORMAT format);
	RenderTarget(int width, int height, int numViews, bool hasDepth);
	~RenderTarget();

	void Initialize(int width, int height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ, DXGI_FORMAT format);
	void Initialize(int width, int height, int numViews, bool hasDepth);
	void InitializeCube(int size, int numViews, bool hasDepth, DXGI_FORMAT format);
	void InitializeCube(int size, int numViews, bool hasDepth);
	void Activate(ID3D11DeviceContext*);
	void Activate(ID3D11DeviceContext* context, float r, float g, float b, float a);
	void Activate(ID3D11DeviceContext* context, DepthTarget*, float r, float g, float b, float a);
	void Activate(ID3D11DeviceContext* context, DepthTarget*);
	void Set(ID3D11DeviceContext* context);
	void Set(ID3D11DeviceContext* context, DepthTarget*);
	void Retarget(ID3D11ShaderResourceView* resource);
	void Restore();
};

