#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"


class wiDepthTarget;

class wiRenderTarget
{
private:
	int numViews;
	void clear();
	vector<wiGraphicsTypes::Texture2D*>		renderTargets;
	vector<wiGraphicsTypes::TextureCube*>	renderTargets_Cube;
	bool isCube;
public:
	wiGraphicsTypes::ViewPort	viewPort;
	wiDepthTarget*				depth;

	wiRenderTarget();
	wiRenderTarget(UINT width, UINT height, bool hasDepth = false, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, UINT MSAAC = 1, bool depthOnly = false);
	~wiRenderTarget();

	void Initialize(UINT width, UINT height, bool hasDepth = false, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, UINT MSAAC = 1, bool depthOnly = false);
	void InitializeCube(UINT size, bool hasDepth, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, bool depthOnly = false);
	void Add(wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM);
	void Activate(GRAPHICSTHREAD threadID, bool disableColor = false);
	void Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor = false);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*, float r, float g, float b, float a, bool disableColor = false);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false);
	void Deactivate(GRAPHICSTHREAD threadID);
	void Set(GRAPHICSTHREAD threadID, bool disableColor = false);
	void Set(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false);

	wiGraphicsTypes::Texture2D* GetTexture(int viewID = 0) const{ return (isCube ? renderTargets_Cube[viewID] : renderTargets[viewID]); }
	wiGraphicsTypes::Texture2DDesc GetDesc(int viewID = 0) const { assert(viewID < numViews); return GetTexture(viewID)->GetDesc(); }
	UINT GetMipCount();
	bool IsInitialized() const { return (numViews > 0 || depth != nullptr); }
};

