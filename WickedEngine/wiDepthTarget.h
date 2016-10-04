#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiDepthTarget
{
private:
	wiGraphicsTypes::Texture2D*		texture;
public:

	wiDepthTarget();
	~wiDepthTarget();
	
	void Initialize(int width, int height, UINT MSAAC);
	void InitializeCube(int size, bool independentFaces = false);
	void Clear(GRAPHICSTHREAD threadID);
	void CopyFrom(const wiDepthTarget&, GRAPHICSTHREAD threadID);

	wiGraphicsTypes::Texture2D* GetTexture() const { return texture; }
	wiGraphicsTypes::Texture2DDesc GetDesc() const { return GetTexture()->GetDesc(); }
};

