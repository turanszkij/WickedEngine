#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

#include <vector>
#include <memory>

class wiDepthTarget;

class wiRenderTarget
{
private:
	struct RenderTargetSlot
	{
		wiGraphicsTypes::Texture2D target_primary;
		wiGraphicsTypes::Texture2D target_resolved;
		bool dirty = true;
	};
	std::vector<RenderTargetSlot> slots;
public:
	wiGraphicsTypes::ViewPort viewPort;
	std::unique_ptr<wiDepthTarget> depth;

	void Initialize(UINT width, UINT height, bool hasDepth = false, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, UINT MSAAC = 1, bool depthOnly = false);
	void InitializeCube(UINT size, bool hasDepth, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM, UINT mipMapLevelCount = 1, bool depthOnly = false);
	void Add(wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM);
	void SetAndClear(GRAPHICSTHREAD threadID, bool disableColor = false, int viewID = -1);
	void SetAndClear(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor = false, int viewID = -1);
	void SetAndClear(GRAPHICSTHREAD threadID, wiDepthTarget*, float r, float g, float b, float a, bool disableColor = false, int viewID = -1);
	void SetAndClear(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false, int viewID = -1);
	void Deactivate(GRAPHICSTHREAD threadID);
	void Set(GRAPHICSTHREAD threadID, bool disableColor = false, int viewID = -1);
	void Set(GRAPHICSTHREAD threadID, wiDepthTarget*, bool disableColor = false, int viewID = -1);

	const wiGraphicsTypes::Texture2D& GetTexture(int viewID = 0) const { return slots[viewID].target_primary; }
	const wiGraphicsTypes::Texture2D& GetTextureResolvedMSAA(GRAPHICSTHREAD threadID, int viewID = 0);
	wiGraphicsTypes::TextureDesc GetDesc(int viewID = 0) const { return GetTexture(viewID).GetDesc(); }
	UINT GetMipCount();
	bool IsInitialized() const { return (!slots.empty() || depth != nullptr); }
};

