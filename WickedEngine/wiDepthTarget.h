#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

#include <memory>

class wiDepthTarget
{
private:
	wiGraphicsTypes::Texture2D target_primary;
	wiGraphicsTypes::Texture2D target_resolved;
	bool dirty = true;
public:
	
	void Initialize(int width, int height, UINT MSAAC);
	void InitializeCube(int size, bool independentFaces = false);
	void Clear(GRAPHICSTHREAD threadID);
	void CopyFrom(const wiDepthTarget&, GRAPHICSTHREAD threadID);

	const wiGraphicsTypes::Texture2D& GetTexture() const { return target_primary; }
	const wiGraphicsTypes::Texture2D& GetTextureResolvedMSAA(GRAPHICSTHREAD threadID);
	const wiGraphicsTypes::TextureDesc& GetDesc() const { return GetTexture().GetDesc(); }
};

