#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

#include <vector>

class wiDepthTarget;

class wiRenderTarget
{
private:
	int numViews;
	std::vector<wiGraphicsTypes::Texture2D*>		renderTargets;
	std::vector<wiGraphicsTypes::Texture2D*>		renderTargets_resolvedMSAA;
	std::vector<int>								resolvedMSAAUptodate;
public:
	wiGraphicsTypes::ViewPort	viewPort;
	wiDepthTarget*				depth;

	wiRenderTarget();
	wiRenderTarget(UINT width, UINT height, bool hasDepth = false, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, UINT MSAAC = 1, bool depthOnly = false);
	~wiRenderTarget();
	void CleanUp();

	void Initialize(UINT width, UINT height, bool hasDepth = false, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, UINT MSAAC = 1, bool depthOnly = false);
	void InitializeCube(UINT size, bool hasDepth, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, bool depthOnly = false);
	void Add(wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM);
	void Activate(GRAPHICSTHREAD threadID, bool disableColor = false, int viewID = -1);
	void Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor = false, int viewID = -1);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*, float r, float g, float b, float a, bool disableColor = false, int viewID = -1);
	void Activate(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false, int viewID = -1);
	void Deactivate(GRAPHICSTHREAD threadID);
	void Set(GRAPHICSTHREAD threadID, bool disableColor = false, int viewID = -1);
	void Set(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false, int viewID = -1);

	wiGraphicsTypes::Texture2D* GetTexture(int viewID = 0) const { return renderTargets[viewID]; }
	wiGraphicsTypes::Texture2D* GetTextureResolvedMSAA(GRAPHICSTHREAD threadID, int viewID = 0);
	wiGraphicsTypes::TextureDesc GetDesc(int viewID = 0) const { assert(viewID < numViews); return GetTexture(viewID)->GetDesc(); }
	UINT GetMipCount();
	bool IsInitialized() const { return (numViews > 0 || depth != nullptr); }
};

