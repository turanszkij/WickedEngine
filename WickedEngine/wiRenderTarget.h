#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"


class wiDepthTarget;

class wiRenderTarget
{
private:
	int numViews;
	//bool retargetted;
	//vector<Texture2D*>	SAVEDshaderResource;
	void clear();
	//Texture2DDesc				textureDesc;
	vector<wiGraphicsTypes::Texture2D*>	renderTargets;
	vector<wiGraphicsTypes::TextureCube*>	renderTargets_Cube;
	bool isCube;
public:
	//vector<Texture2D>			texture2D;
	//vector<RenderTargetView>		renderTarget;
	//vector<Texture2D*>	shaderResource;
	wiGraphicsTypes::ViewPort	viewPort;
	wiDepthTarget*				depth;

	wiRenderTarget();
	wiRenderTarget(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	~wiRenderTarget();

	void Initialize(UINT width, UINT height, int numViews = 1, bool hasDepth = false, UINT MSAAC = 1, UINT MSAAQ = 0, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1);
	void InitializeCube(UINT size, int numViews, bool hasDepth, wiGraphicsTypes::FORMAT format, UINT mipMapLevelCount = 1);
	void InitializeCube(UINT size, int numViews, bool hasDepth);
	void Activate(GRAPHICSTHREAD threadID);
	void Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*, float r, float g, float b, float a);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*);
	void Deactivate(GRAPHICSTHREAD threadID);
	void Set(GRAPHICSTHREAD threadID);
	void Set(GRAPHICSTHREAD threadID, wiDepthTarget*);
	//void Retarget(Texture2D* resource);
	//void Restore();

	wiGraphicsTypes::Texture2D* GetTexture(int viewID = 0) const{ return (isCube ? renderTargets_Cube[viewID] : renderTargets[viewID]); }
	wiGraphicsTypes::Texture2DDesc GetDesc(int viewID = 0) const { assert(viewID < numViews); return GetTexture(viewID)->GetDesc(); }
	UINT GetMipCount();
	bool IsInitialized() { return (numViews > 0 || depth != nullptr); }
};

